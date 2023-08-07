/* TCPfile.c - main, TCPfile */

#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/*extern int	errno;*/
#include <errno.h>

void TCPfile(const char *host, const char *service);
int	errexit(const char *format, ...);

int	connectsock(const char *host, const char *service, const char *transport);

#define	LINELEN		128

/*------------------------------------------------------------------------
 * main - TCP client for ECHO service
 *------------------------------------------------------------------------
 */
int main(int argc, char *argv[])
{
	char	*host = "localhost";	/* host to use if none supplied	*/
	char	*service = "echo";	/* default service name		*/

	switch (argc) {
	case 1:
		host = "localhost";
		break;
	case 3:
		service = argv[2];
		/* FALL THROUGH */
	case 2:
		host = argv[1];
		break;
	default:
		fprintf(stderr, "usage: TCPfile [host [port]]\n");
		exit(1);
	}

	TCPfile(host, service);

	exit(0);
}

/*------------------------------------------------------------------------
 * TCPfile - send input to ECHO service on specified host and print reply
 *------------------------------------------------------------------------
 */
void TCPfile(const char *host, const char *service)
{
	char buf[BUFSIZ];	/* buffer for one line of text	*/
	char filename[LINELEN+1];
	FILE* fileptr;
	int N;
	int	s, n;			/* socket descriptor, read count*/
	int	outchars, inchars;	/* characters sent and received	*/
	struct sockaddr_in serveraddr;
	socklen_t len;

	s = 	connectsock(host, service, "tcp");
	if (s < 0)
	  {
		fprintf(stderr, "usage: TCPfile connectsock failed. \n");
		exit(1);
	  }
	
	len = sizeof(serveraddr);
	getpeername(s, (struct sockaddr*)&serveraddr, &len);

	printf("************** \n");
	printf("Server IP address: %s\n", inet_ntoa(serveraddr.sin_addr));
	printf("Server port      : %d\n", ntohs(serveraddr.sin_port));
	printf("************** \n");


	/*
	while (fgets(buf, sizeof(buf), stdin)) {
		buf[LINELEN] = '\0';	/* insure line null-terminated
		outchars = strlen(buf);
		(void) write(s, buf, outchars);

		/* read it back
		for (inchars = 0; inchars < outchars; inchars+=n ) {
			n = read(s, &buf[inchars], outchars - inchars);
			if (n < 0)
				errexit("socket read failed: %s\n",
					strerror(errno));
		}
		fputs(buf, stdout);
	}
	*/

	fgets(buf, sizeof(buf), stdin);
	sscanf(buf, "%s %d", filename, &N);
	(void) write(s, buf, strlen(buf));

	n = read(s, buf, BUFSIZ);

	if(strcmp(buf, "SORRY!") == 0)
	{
		printf("Server says that the file does not exist.\n");
	}
	else
	{
		printf("Server returned...\n");
		n = printf("%s\n", buf)-1;

		n = strlen(filename);
		filename[n] = '1';
		filename[n+1] = '\0';

		fileptr = fopen(filename, "w");
		fwrite(buf, N, 1, fileptr);
		fclose(fileptr);
	}

	return;
}