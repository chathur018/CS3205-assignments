// NAME: Chathur Bommineni
// Roll Number: CS20B018
// Course: CS3205 Jan. 2023 semester
// Lab number: 2
// Date of submission: 4/3/2023
// I confirm that the source file is entirely written by me without
// resorting to any dishonest means.
// Website(s) that I used for basic socket programming code are:
// URL(s): geeksforgeeks.org ; (my own lab1 code)

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

#define N 64

#include <errno.h>
int	errexit(const char *format, ...);

int connectsock(const char *host, const char *service, const char *transport );

int launchServers(char* inputfile, unsigned short port);
int dnsClient(char* host, unsigned short startport);


int main(int argc, char *argv[])
{
    if(argc < 3)
    {
        printf("usage: abc [port] [inputfile]");
        exit(0);
    }

    unsigned short startport;
    sscanf(argv[1], "%hu", &startport);

    printf("Starting Servers...\n");

    pid_t childpid;
    int status;
    switch (childpid = fork()) {

    case 0:		/* child */
        exit(launchServers(argv[2], startport));

    default:	/* parent */
        waitpid(childpid, &status, 0);
        break;

    case -1:
        errexit("fork: %s\n", strerror(errno));
    }

    dnsClient("localhost", startport);
    kill(0, SIGTERM);
    return 0;
}

int dnsClient(char* host, unsigned short startport)
{
    struct sockaddr_in serveraddr;
	socklen_t len;
    char port[N];

    startport += 53;
    sprintf(port, "%hu", startport);
    int s = connectsock(host, port, "udp");

    len = sizeof(serveraddr);
	getpeername(s, (struct sockaddr*)&serveraddr, &len);

	printf("************** \n");
	printf("Name resolver IP address: %s\n", inet_ntoa(serveraddr.sin_addr));
	printf("Name resolver port      : %d\n", ntohs(serveraddr.sin_port));
	printf("************** \n");

    while(1)
    {
        printf("Enter Server Name: ");
        char query[N];
        scanf("%s", query);
        //printf("%s\n", query);

        if(strcmp(query, "bye")==0)
        {
            printf("Killing server processes");
            break;
        }
        
        write(s, query, strlen(query));
        int n = read(s, query, N);
        if (n < 0)
        {
			printf("socket read failed\n");
            break;
        }
        query[n] = '\0';

        if(strcmp(query, "fail")==0)
            printf("No DNS Record Found\n");
        else
            printf("DNS Mapping: %s\n", query);
    }

    return 0;
}