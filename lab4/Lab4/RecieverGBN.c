// NAME: Chathur Bommineni
// Roll Number: CS20B018
// Course: CS3205 Jan. 2023 semester
// Lab number: 4
// Date of submission: 5-4-2023
// I confirm that the source file is entirely written by me without
// resorting to any dishonest means.
// Website(s) that I used for basic socket programming code are:
// URL(s): geeksforgeeks.com

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int	errexit(const char *format, ...);

int connectsock(const char *host, const char *service, const char *transport );
int passivesock(const char *service, const char *transport, int qlen);


bool debugMode = false;
char* addr = "localhost";
char* port = "time";
int maxPackets = 1024;
float dropRate = 0.2;

int main(int argc, char *argv[])
{
    srand(time(0));
    
    int commands = 1;
    while(commands < argc)
    {
        if(strcmp(argv[commands], "-d")==0)
        {
            debugMode = true;
            commands--;
        }
        else if(strcmp(argv[commands], "-p")==0)
        {
            port = argv[commands+1];
        }
        else if(strcmp(argv[commands], "-n")==0)
        {
            sscanf(argv[commands+1], "%d", &maxPackets);
        }
        else if(strcmp(argv[commands], "-e")==0)
        {
            sscanf(argv[commands+1], "%f", &dropRate);
        }
        
        commands += 2;
    }
    dropRate *= 100;

    char buffer[1024];
    int num = 0;

    struct sockaddr_in fsin;
    unsigned int alen;
    
    int socketRecv = passivesock(port, "udp", 0);

    int socketSend = connectsock(addr, port, "udp");

    while(num < maxPackets)
    {
        alen = sizeof(fsin);
        int count = recvfrom(socketRecv, buffer, sizeof(buffer), 0, (struct sockaddr *)&fsin, &alen);
        uint8_t seq = buffer[0];
        //printf("a");
        if(debugMode)
            printf("Seq %hhu: Time Recieved: %li ", seq, clock()*1000000/CLOCKS_PER_SEC);

        double error = (double)rand() / (double) RAND_MAX;
        if(seq != num%256)
        {
            //printf("b %hhu %d\n", seq, num%256);
            if(debugMode)
                printf("Packet dropped: true\n");
            continue;
        }
        if(error > dropRate)
        {
            sendto(socketRecv, buffer, 1, 0, (struct sockaddr *)&fsin, sizeof(fsin));
            num++;
            //sleep(1);
            if(debugMode)
                printf("Packet dropped: false\n");
        }
        else
        {
            //printf("a\n");
            if(debugMode)
                printf("Packet dropped: true\n");
        }
    }

    return 0;
}