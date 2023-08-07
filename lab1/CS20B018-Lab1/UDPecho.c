/* UDPecho.c - main */

#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#define	LINELEN 128

/*extern int	errno;*/
#include <errno.h>

int	errexit(const char *format, ...);

int connectsock(const char *host, const char *service, const char *transport );

/*------------------------------------------------------------------------
 * main - UDP client for TIME service that prints the resulting time
 *------------------------------------------------------------------------
 */

int main(int argc, char *argv[])
{
	char	*host = "localhost";	/* host to use if none supplied	*/
	char	*service = "time";	/* default service name		*/
	time_t	now;			/* 32-bit integer to hold time	*/ 
	int	s, n;			/* socket descriptor, read count*/
	struct sockaddr_in serveraddr;
	socklen_t len;
	char buf[LINELEN+1];
	int outchars, inchars;
 
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
		fprintf(stderr, "usage: UDPecho [host [port]]\n");
		exit(1);
	}

	s = connectsock(host, service, "udp");

	len = sizeof(serveraddr);
	getpeername(s, (struct sockaddr*)&serveraddr, &len);

	printf("************** \n");
	printf("Server IP address: %s\n", inet_ntoa(serveraddr.sin_addr));
	printf("Server port      : %d\n", ntohs(serveraddr.sin_port));
	printf("************** \n");
	
	while(fgets(buf, sizeof(buf), stdin))
	{
		outchars = strlen(buf);
		buf[outchars-1] = '\0';
		(void) write(s, buf, outchars);

		for (inchars = 0; inchars < outchars; inchars+=n ) {
			n = read(s, &buf[inchars], outchars - inchars);
			if (n < 0)
				errexit("socket read failed: %s\n", strerror(errno));
		}
		printf("%s\n", buf);
	}

	exit(0);
}
