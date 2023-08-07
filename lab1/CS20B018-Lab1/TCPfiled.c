/* TCPfiled.c - main, TCPfiled */

#define	_USE_BSD
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/signal.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/errno.h>
#include <netinet/in.h>

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define	QLEN		  32	/* maximum connection queue length	*/
#define	BUFSIZE		4096
#define LINELEN		128

/*extern int	errno;*/
#include <errno.h>

void	reaper(int);
int	TCPfiled(int fd);
int	errexit(const char *format, ...);
int	passivesock(const char *service, const char *transport, int qlen);

/*------------------------------------------------------------------------
 * main - Concurrent TCP server for ECHO service
 *------------------------------------------------------------------------
 */
int main(int argc, char *argv[])
{
	char	*service = "echo";	/* service name or port number	*/
	struct	sockaddr_in fsin;	/* the address of a client	*/
	unsigned int	alen;		/* length of client's address	*/
	int	msock;			/* master server socket		*/
	int	ssock;			/* slave server socket		*/

	switch (argc) {
	case	1:
		break;
	case	2:
		service = argv[1];
		break;
	default:
		errexit("usage: TCPfiled [port]\n");
	}

	msock = passivesock(service, "tcp", QLEN);

	(void) signal(SIGCHLD, reaper);

	while (1) {
	  alen = sizeof(fsin);
	  ssock = accept(msock, (struct sockaddr *)&fsin, &alen);

	  if (ssock < 0) {
			if (errno == EINTR)
				continue;
			errexit("accept: %s\n", strerror(errno));
		}

	  switch (fork()) {

	  case 0:		/* child */
	    (void) close(msock);
	    exit(TCPfiled(ssock));

	  default:	/* parent */
	    (void) close(ssock);
	    break;

		case -1:
			errexit("fork: %s\n", strerror(errno));
		}
	}
}

/*------------------------------------------------------------------------
 * TCPfiled - echo data until end of file
 *------------------------------------------------------------------------
 */
int TCPfiled(int s)
{
	char	buf[BUFSIZ];
	int	cc;
	struct sockaddr_in clientaddr;
	socklen_t len;
	char filename[LINELEN+1];
	int N;
	FILE* fileptr;

	len = sizeof(clientaddr);
	getpeername(s, (struct sockaddr*)&clientaddr, &len);

	printf("************** \n");
	printf("Client IP address: %s\n", inet_ntoa(clientaddr.sin_addr));
	printf("Client port      : %d\n", ntohs(clientaddr.sin_port));
	

	cc = read(s, buf, sizeof buf);
	if (cc < 0)
		errexit("echo read: %s\n", strerror(errno));
	sscanf(buf, "%s %d", filename, &N);
	printf("Client requested : %s, %d bytes\n", filename, N);

	fileptr = fopen(filename, "r");

	if(fileptr == NULL)
	{
		cc = sprintf(buf, "SORRY!");
		printf("File not found...\n");
		(void) write(s, buf, cc+1);
	}
	else
	{
		fseek(fileptr, 0, SEEK_END);

		cc = ftell(fileptr);
		printf("Sending %d bytes...\n", N);

		fseek(fileptr, N*(-1)-1, SEEK_CUR);

		fread(buf, N, 1, fileptr);
		buf[N] = '\0';
		(void) write(s, buf, N+1);

		fclose(fileptr);
	}
	printf("************** \n");
	return 0;
}

/*------------------------------------------------------------------------
 * reaper - clean up zombie children
 *------------------------------------------------------------------------
 */
void reaper(int sig)
{
	int	status;

	while (wait3(&status, WNOHANG, (struct rusage *)0) >= 0)
		/* empty */;
}
