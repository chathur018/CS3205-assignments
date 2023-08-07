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
int passivesock(const char *service, const char *transport, int qlen);

struct addrList
{
    char list[6][2][N];
    char port[6][N];
};

int nameResolver(char* ip, unsigned short port, struct addrList l);
int rootDNSServer(char* ip, unsigned short port, struct addrList l);
int tldServer(char* ip, unsigned short port, char* domain, struct addrList l);
int authServer(char* ip, unsigned short port, char* domain, struct addrList l);

int launchServers(char* inputfile, unsigned short startport)
{

    FILE* fileptr;
    fileptr = fopen(inputfile, "r");

    if(fileptr == NULL)
    {
        printf("input file not found\n");
        exit(0);
    }

    struct addrList curr;
    struct addrList prev;

    char buffer[N];
    char name[N];
    char val[N];
    int n;

    fgets(buffer, N, fileptr);
    //printf("%s", buffer);

    fgets(buffer, N, fileptr);
    sscanf(buffer, "%s", curr.list[0][0]);
    n = sscanf(buffer+strlen(curr.list[0][0]), "%s", curr.list[0][1]);
    //printf("%s %s\n", curr.list[0][0], curr.list[0][1]);

    //nameserver
    prev = curr;
    fgets(buffer, N, fileptr);
    n = sscanf(buffer, "%s", curr.list[0][0]);
    n = sscanf(buffer+strlen(curr.list[0][0]), "%s", curr.list[0][1]);
    //printf("%s %s\n", curr.list[0][0], curr.list[0][1]);
    switch (fork()) {
    case 0:		/* child */
        exit(nameResolver(prev.list[0][1], startport+53, curr));
    default:	/* parent */
        break;
    case -1:
        errexit("fork: %s\n", strerror(errno));
    }

    //rootserver
    prev = curr;
    for(int i=0; i<2; i++)
    {
        fgets(buffer, N, fileptr);
        n = sscanf(buffer, "%s", curr.list[i][0]);
        n = sscanf(buffer+strlen(curr.list[i][0]), "%s", curr.list[i][1]);
        sprintf(curr.port[i], "%hu", startport+55+i);
    }
    switch (fork()) {
    case 0:		/* child */
        exit(rootDNSServer(prev.list[0][1], startport+54, curr));
    default:	/* parent */
        break;
    case -1:
        errexit("fork: %s\n", strerror(errno));
    }

    //tld servers
    prev = curr;
    for(int i=0; i<6; i++)
    {
        fgets(buffer, N, fileptr);
        n = sscanf(buffer, "%s", curr.list[i][0]);
        n = sscanf(buffer+strlen(curr.list[i][0]), "%s", curr.list[i][1]);
        sprintf(curr.port[i], "%hu", startport+57+i);
    }
    //.com
    switch (fork()) {
    case 0:		/* child */
        exit(tldServer(prev.list[0][1], startport+55, ".com", curr));
    default:	/* parent */
        break;
    case -1:
        errexit("fork: %s\n", strerror(errno));
    }
    //.edu
    switch (fork()) {
    case 0:		/* child */
        exit(tldServer(prev.list[1][1], startport+56, ".edu", curr));
    default:	/* parent */
        break;
    case -1:
        errexit("fork: %s\n", strerror(errno));
    }

    //auth servers
    prev = curr;
    for(int j=0; j<6; j++)
    {
        fgets(buffer, N, fileptr);
        for(int i=0; i<5; i++)
        {
            fgets(buffer, N, fileptr);
            n = sscanf(buffer, "%s", curr.list[i][0]);
            n = sscanf(buffer+strlen(curr.list[i][0]), "%s", curr.list[i][1]);
        }
        switch (fork()) {
        case 0:		/* child */
            exit(authServer(prev.list[j][1], startport+57+j, prev.list[j][0], curr));
        default:	/* parent */
            break;
        case -1:
            errexit("fork: %s\n", strerror(errno));
        }
    }

    fgets(buffer, N, fileptr);
    if(strcmp("END_DATA\n", buffer))
        errexit("invalid inputfile");

    return 0;
}

int nameResolver(char* ip, unsigned short p, struct addrList l)
{
    FILE* fileptr = fopen("NR.output", "w");
    fprintf(fileptr, "Name resolver: %s %hu\n\n", ip, p);
    fclose(fileptr);

    char port[N];
    sprintf(port, "%hu", p);
    char dnsip[N];
    char dnsport[N];
    char query[N*2];
    struct sockaddr_in fsin;
    unsigned int	alen;
	alen = sizeof(fsin);

    int sock1 = passivesock(port, "udp", 0);
    
    char buffer[N*2];
    while(1)
    {
        sprintf(dnsport, "%hu", p+1);
        int count = recvfrom(sock1, buffer, N, 0, (struct sockaddr *)&fsin, &alen);
        buffer[count] = '\0';
        sprintf(query, "%s", buffer);

        fileptr = fopen("NR.output", "a");
        fprintf(fileptr, "Query recieved: %s\n", query);

        fprintf(fileptr, "Querying RDS with: %s\n", query);
        sprintf(dnsport, "%hu", p+1);
        int sock2 = connectsock(l.list[0][1], dnsport, "udp");
        write(sock2, buffer, count);

        count = read(sock2, buffer, N);
        if (count < 0)
        {
			printf("socket read failed\n");
            continue;
        }
        buffer[count] = '\0';
        fprintf(fileptr, "Recieved from RDS: %s\n", buffer);
        if(strcmp(buffer, "fail")==0)
        {
            fprintf(fileptr, "Returning: %s\n\n", buffer);
            fclose(fileptr);
            sendto(sock1, buffer, count, 0, (struct sockaddr *)&fsin, sizeof(fsin));
            continue;
        }
        sscanf(buffer, "%s %s", dnsip, dnsport);

        fprintf(fileptr, "Querying TLD with: %s\n", query);
        int sock3 = connectsock(dnsip, dnsport, "udp");
        write(sock3, query, strlen(query));

        count = read(sock3, buffer, N);
        if (count < 0)
        {
			printf("socket read failed\n");
            continue;
        }
        buffer[count] = '\0';
        fprintf(fileptr, "Recieved from TLD: %s\n", buffer);
        if(strcmp(buffer, "fail")==0)
        {
            fprintf(fileptr, "Returning: %s\n\n", buffer);
            fclose(fileptr);
            sendto(sock1, buffer, count, 0, (struct sockaddr *)&fsin, sizeof(fsin));
            continue;
        }
        sscanf(buffer, "%s %s", dnsip, dnsport);

        fprintf(fileptr, "Querying ADS with: %s\n", query);
        int sock4 = connectsock(dnsip, dnsport, "udp");
        write(sock4, query, strlen(query));

        count = read(sock4, buffer, N);
        if (count < 0)
        {
			printf("socket read failed\n");
            continue;
        }
        buffer[count] = '\0';
        fprintf(fileptr, "Recieved from ADS: %s\n", buffer);
        if(strcmp(buffer, "fail")==0)
        {
            fprintf(fileptr, "Returning: %s\n\n", buffer);
            fclose(fileptr);
            sendto(sock1, buffer, count, 0, (struct sockaddr *)&fsin, sizeof(fsin));
            continue;
        }
        
        fprintf(fileptr, "Returning: %s\n\n", buffer);
        fclose(fileptr);
        sendto(sock1, buffer, count, 0, (struct sockaddr *)&fsin, sizeof(fsin));
    }

    return 0;
}

int rootDNSServer(char* ip, unsigned short p, struct addrList l)
{
    FILE* fileptr = fopen("RDS.output", "w");
    fprintf(fileptr, "Root DNS Server: %s %hu\n\n", ip, p);
    fclose(fileptr);

    char port[N];
    sprintf(port, "%hu", p);
    struct sockaddr_in fsin;
    unsigned int	alen;
	alen = sizeof(fsin);

    int sock = passivesock(port, "udp", 0);

    char buffer[N*2];
    char *fail = "fail";
    while(1)
    {
        int count = recvfrom(sock, buffer, N, 0, (struct sockaddr *)&fsin, &alen);
        buffer[count] = '\0';

        fileptr = fopen("RDS.output", "a");
        fprintf(fileptr, "Query recieved: %s\n", buffer);

        int pos = -1;
        for(size_t i=0; i<=strlen(buffer); i++)
            if(buffer[i] == '.')
                pos = i;
        
        for(int i=0; i<2; i++)
        {
            if(strcmp(buffer+pos+1, l.list[i][0]+4)==0)
            {
                sprintf(buffer, "%s %s", l.list[i][1], l.port[i]);
                break;
            }

            if(i==1)
                sprintf(buffer, "%s", fail);
        }

        fprintf(fileptr, "Returning: %s\n\n", buffer);
        fclose(fileptr);

        sendto(sock, buffer, strlen(buffer), 0, (struct sockaddr *)&fsin, sizeof(fsin));
    }

    return 0;
}

int tldServer(char* ip, unsigned short p, char* domain, struct addrList l)
{
    char outfile[N];
    int start;
    if(strcmp(domain, ".com") == 0)
    {
        sprintf(outfile, "TDS_com.output");
        start = 0;
    }
    else if(strcmp(domain, ".edu") == 0)
    {
        sprintf(outfile, "TDS_edu.output");
        start = 3;
    }
    
    FILE* fileptr = fopen(outfile, "w");
    fprintf(fileptr, "TLD %s Server: %s %hu\n\n", domain, ip, p);
    //fprintf(fileptr, "%s %s\n", l.list[start][0], l.list[start][1]);
    fclose(fileptr);

    char port[N];
    sprintf(port, "%hu", p);
    struct sockaddr_in fsin;
    unsigned int	alen;
	alen = sizeof(fsin);

    int sock = passivesock(port, "udp", 0);

    char buffer[N*2];
    char *fail = "fail";
    while(1)
    {
        int count = recvfrom(sock, buffer, N, 0, (struct sockaddr *)&fsin, &alen);
        buffer[count] = '\0';

        fileptr = fopen(outfile, "a");
        fprintf(fileptr, "Query recieved: %s\n", buffer);

        int pos = -1;
        int prev = -1;
        for(size_t i=0; i<=strlen(buffer); i++)
            if(buffer[i] == '.')
            {
                pos = prev;
                prev = i;
            }
        
        for(int i=start; i<start+3; i++)
        {
            if(strcmp(buffer+pos+1, l.list[i][0])==0)
            {
                sprintf(buffer, "%s %s", l.list[i][1], l.port[i]);
                break;
            }

            if(i==start+2)
                sprintf(buffer, "%s", fail);
        }

        fprintf(fileptr, "Returning: %s\n\n", buffer);
        fclose(fileptr);

        sendto(sock, buffer, strlen(buffer), 0, (struct sockaddr *)&fsin, sizeof(fsin));
    }

    return 0;
}

int authServer(char* ip, unsigned short p, char* domain, struct addrList l)
{
    char outfile[N];
    sprintf(outfile, "ADS_%s.output", domain);
    
    FILE* fileptr = fopen(outfile, "w");
    fprintf(fileptr, "ADS %s Server: %s %hu\n\n", domain, ip, p);
    //fprintf(fileptr, "%s %s\n", l.list[4][0], l.list[4][1]);
    fclose(fileptr);

    char port[N];
    sprintf(port, "%hu", p);
    struct sockaddr_in fsin;
    unsigned int	alen;
	alen = sizeof(fsin);

    int sock = passivesock(port, "udp", 0);

    char buffer[N*2];
    char *fail = "fail";
    while(1)
    {
        int count = recvfrom(sock, buffer, N, 0, (struct sockaddr *)&fsin, &alen);
        buffer[count] = '\0';

        fileptr = fopen(outfile, "a");
        fprintf(fileptr, "Query recieved: %s\n", buffer);

        for(int i=0; i<5; i++)
        {
            if(strcmp(buffer, l.list[i][0])==0)
            {
                sprintf(buffer, "%s", l.list[i][1]);
                break;
            }

            if(i==4)
                sprintf(buffer, "%s", fail);
        }

        fprintf(fileptr, "Returning: %s\n\n", buffer);
        fclose(fileptr);

        sendto(sock, buffer, strlen(buffer), 0, (struct sockaddr *)&fsin, sizeof(fsin));
    }

    return 0;
}