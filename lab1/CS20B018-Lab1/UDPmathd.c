/* UDPmathd.c - main */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

/*extern int	errno;*/
#include <errno.h>
#include <math.h>

#define LINELEN 128

int	passiveUDP(const char *service);
int	errexit(const char *format, ...);

int passivesock(const char *service, const char *transport, int qlen);

int hyp(int a, int b)
{
	int res = a*a;
	res += b*b;
	res = (int)sqrt((double)res);
	return res;
}

/*------------------------------------------------------------------------
 * main - Iterative UDP server for TIME service
 *------------------------------------------------------------------------
 */
int main(int argc, char *argv[])
{
	struct sockaddr_in fsin;	/* the from address of a client	*/
	char	*service = "time";	/* service name or port number	*/
	char	buf[BUFSIZ];			/* "input" buffer; any size > 0	*/
	int	sock;			/* server socket		*/
	time_t	now;			/* current time			*/
	unsigned int	alen;		/* from-address length		*/
	char operator[LINELEN];
	int op1, op2;
	int res;

	switch (argc) {
	case	1:
		break;
	case	2:
		service = argv[1];
		break;
	default:
		errexit("usage: UDPmathd [port]\n");
	}

	sock = passivesock(service, "udp", 0);
	/* Last parameter is Queue length and not applicable for UDP sockets*/

	while (1) {
	  int count;
	  alen = sizeof(fsin);

	  count = recvfrom(sock, buf, sizeof(buf), 0, (struct sockaddr *)&fsin, &alen);

	  printf("************** \n");
	  printf("Client IP address: %s\n", inet_ntoa(fsin.sin_addr));
	  printf("Client port      : %d\n", ntohs(fsin.sin_port));
	  
	  if (count < 0)
	    errexit("recvfrom: %s\n", strerror(errno));
	  
	  count = sscanf(buf, "%s %d %d", operator, &op1, &op2);
	  operator[3] = '\0';
	  /*printf("%s\n%d\n%d\n", operator, op1, op2);*/
	  printf("Recieved         : %s %d %d\n", operator, op1, op2);

	  /*printf("Client sent      : %s\n", buf);*/

	  if(strcmp(operator, "add")==0)
	  {
		res = op1 + op2;
	  }else if(strcmp(operator, "mul")==0)
	  {
		res = op1 * op2;
	  }else if(strcmp(operator, "mod")==0)
	  {
		res = op1 % op2;
	  }else if(strcmp(operator, "hyp")==0)
	  {
		res = hyp(op1, op2);
	  }else
	  {
		count = sprintf(buf, "Error : operator not found");
	    printf("************** \n");
		(void) sendto(sock, buf, count+1, 0, (struct sockaddr *)&fsin, sizeof(fsin));
		continue;
	  }

	  count = sprintf(buf, "%d", res);

	  if(count < BUFSIZ)
	  	buf[count] = '\0';
	  else
	  	buf[BUFSIZ-1] = '\0';

	  (void) sendto(sock, buf, count+1, 0, (struct sockaddr *)&fsin, sizeof(fsin));

	  printf("************** \n");
	}
}
