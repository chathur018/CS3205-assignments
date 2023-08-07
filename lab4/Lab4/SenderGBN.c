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
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <errno.h>

int	errexit(const char *format, ...);

int connectsock(const char *host, const char *service, const char *transport );


struct pktBuffer
{
    char* address;
    int pushSlot;
    int popSlot;
    int pktLen;
};

void *packetGenerator(void *arg);
void *packetSender(void *arg);
void *recvAcks(void *arg);
void *timeoutManager(void *arg);


bool debugMode = false;
int packetLength = 64;
int packetGenRate = 1;
int maxBufferSize = 64;
int maxPackets = 1024;
int windowSize = 32;
int currentPkt = 0;
int countPkts = 0;
int nextGenPkt = 0;
int windowStart = 0;
int totalTransmission = 0;
double rttavg = 0;

struct pktBuffer buffer;
bool *ackWaiting;
long int *timeouts;
long int *startTimes;

char* addr = "localhost";
char* port = "time";

int socketSend;

pthread_mutex_t lock[8];

int main(int argc, char *argv[])
{
    int commands = 1;
    while(commands < argc)
    {
        if(strcmp(argv[commands], "-d")==0)
        {
            debugMode = true;
            commands--;
        }
        else if(strcmp(argv[commands], "-s")==0)
        {
            addr = argv[commands+1];
        }
        else if(strcmp(argv[commands], "-p")==0)
        {
            port = argv[commands+1];
        }
        else if(strcmp(argv[commands], "-l")==0)
        {
            sscanf(argv[commands+1], "%d", &packetLength);
        }
        else if(strcmp(argv[commands], "-r")==0)
        {
            sscanf(argv[commands+1], "%d", &packetGenRate);
        }
        else if(strcmp(argv[commands], "-n")==0)
        {
            sscanf(argv[commands+1], "%d", &maxPackets);
        }
        else if(strcmp(argv[commands], "-w")==0)
        {
            sscanf(argv[commands+1], "%d", &windowSize);
        }
        else if(strcmp(argv[commands], "-f")==0)
        {
            sscanf(argv[commands+1], "%d", &maxBufferSize);
        }
        
        commands += 2;
    }

    buffer.pktLen = packetLength;
    buffer.address = (char*)malloc(buffer.pktLen*maxBufferSize);
    buffer.popSlot = maxBufferSize-1;
    buffer.pushSlot = 0;

    ackWaiting = (bool*)malloc(256*sizeof(bool));
    memset(ackWaiting, 0, 256*sizeof(bool));
    timeouts = (long int*)malloc(256*sizeof(long int));
    startTimes = (long int*)malloc(256*sizeof(long int));
    
    for(int i=0; i<8; i++)
        if (pthread_mutex_init(&lock[i], NULL) != 0)
        {
            printf("\n mutex init has failed\n");
            return 1;
        }

    pthread_t pktgenThreadid;
    pthread_create(&pktgenThreadid, NULL, packetGenerator, NULL);

    pthread_mutex_lock(&lock[5]);
    pthread_t pktsendThreadid;
    pthread_create(&pktsendThreadid, NULL, packetSender, NULL);

    pthread_mutex_lock(&lock[3]);
    pthread_t pktrecvThreadid;
    pthread_create(&pktrecvThreadid, NULL, recvAcks, NULL);

    pthread_t timeoutThreadid;
    pthread_create(&timeoutThreadid, NULL, timeoutManager, NULL);

    pthread_join(pktsendThreadid, NULL);

    printf("PACKET_GEN_RATE: %d\n", packetGenRate);
    printf("PACKET_LENGTH: %d\n", packetLength);
    printf("Retrasmission Ratio: %f\n", (float)totalTransmission/maxPackets);
    printf("Average RTT: %f\n", (float)rttavg/maxPackets);
    return 0;
}

void addPktToBuffer(char* packet)
{
    pthread_mutex_lock(&lock[0]);

    int dist = (buffer.popSlot-buffer.pushSlot)%maxBufferSize;
    if(dist < 0)
        dist += maxBufferSize;

    //printf("%d\n", i);

    if(dist > 1)
    {
        char* addr = buffer.address+buffer.pushSlot*buffer.pktLen;
        memcpy(addr, packet, packetLength);
        *addr = nextGenPkt++;
        nextGenPkt %= 256;
        buffer.pushSlot++;
        buffer.pushSlot %= maxBufferSize;
    }

    pthread_mutex_unlock(&lock[0]);
}

void *packetGenerator(void *arg)
{
    int period = 1000000 / packetGenRate;
    //printf("%lf\n", period);
    char num = 0;
    while(true)
    {
        clock_t start = clock();

        //make and send packet
        char* pkt = (char*)malloc(packetLength);
        if(packetLength > 1)
            *((uint8_t*)(pkt+1)) = num%256;
        addPktToBuffer(pkt);
        num++;

        while((clock()-start) * 1000000 / CLOCKS_PER_SEC < period);
            //usleep(period/1000);
    }
}

void *packetSender(void *arg)
{
    pthread_mutex_lock(&lock[5]);
    int windowEnd = windowSize;

    socketSend = connectsock(addr, port, "udp");

    struct sockaddr_in serveraddr;
    socklen_t len = sizeof(serveraddr);
	getpeername(socketSend, (struct sockaddr*)&serveraddr, &len);

    pthread_mutex_unlock(&lock[3]);

	//printf("************** \n");
	//printf("Server IP address: %s\n", inet_ntoa(serveraddr.sin_addr));
	//printf("Server port      : %d\n", ntohs(serveraddr.sin_port));
	//printf("************** \n");

    do
    {
        pthread_mutex_lock(&lock[1]);
        bool end = countPkts >= maxPackets;
        pthread_mutex_unlock(&lock[1]);

        if(end)
            break;
        
        pthread_mutex_lock(&lock[0]);
        int popPos = buffer.popSlot;
        int pushPos = buffer.pushSlot;
        pthread_mutex_unlock(&lock[0]);
        windowEnd = (popPos+windowSize)%maxBufferSize;

        if(currentPkt == pushPos || currentPkt == windowEnd)
            continue;
        
        //send currentPkt
        pthread_mutex_lock(&lock[2]);
        pthread_mutex_lock(&lock[4]);
        uint8_t *pos = (uint8_t*)buffer.address+currentPkt*buffer.pktLen;
        write(socketSend, pos, buffer.pktLen);
        //printf("sen %hhu\n", *pos);
        //printf("spe %hhu\n", ackWaiting[127]);

        int p = *pos;
        //printf("%d\n", p);
        ackWaiting[p] = true;
        startTimes[p] = (clock()*1000000)/CLOCKS_PER_SEC;
        timeouts[p] = 100000 + startTimes[p];
        //printf("tim %li %hhu\n", timeouts[p], ackWaiting[p]);
        pthread_mutex_unlock(&lock[4]);

        currentPkt++;
        currentPkt %= maxBufferSize;
        totalTransmission++;

        pthread_mutex_unlock(&lock[2]);
    }while(true);
}

void *recvAcks(void *arg)
{
    pthread_mutex_lock(&lock[3]);

    char buf;

    while(true)
    {
        int count = read(socketSend, &buf, 1);
        if(count < 0)
            errexit("socket read failed: %s\n", strerror(errno));
        
        pthread_mutex_lock(&lock[4]);
        uint8_t seq = buf;
        long int rtt = clock()*1000000/CLOCKS_PER_SEC - startTimes[seq];
        rttavg += rtt;
        if(debugMode)
            printf("Seq %hhu: Time Generated: %li RTT: %li\n", buf, startTimes[seq], rtt);
        buffer.popSlot++;
        buffer.popSlot %= maxBufferSize;
        countPkts++;

        ackWaiting[(int)buffer.address[currentPkt*buffer.pktLen]] = false;
        windowStart++;
        if(windowStart == 256)
        {
            windowStart = 0;
            memset(ackWaiting, 0, 256*sizeof(bool));
        }
        pthread_mutex_unlock(&lock[4]);
    }
}

void *timeoutManager(void *arg)
{
    pthread_mutex_unlock(&lock[5]);
    while(true)
    {
        pthread_mutex_lock(&lock[4]);
        int start = windowStart;
        pthread_mutex_unlock(&lock[4]);

        for(int i=start; i<start+windowSize; i++)
        {
            //printf("chk %d", i);
            int j = i%256;
            if(ackWaiting[j] && (clock()*1000000)/CLOCKS_PER_SEC > timeouts[j])
            {
                pthread_mutex_lock(&lock[2]);
                int diff = j - start;
                if(diff < 0)
                    diff += 256;
                currentPkt = (buffer.popSlot + 1 + diff)%maxBufferSize;
                //printf("chg %d pop %d %d\n", j, currentPkt, start);
                pthread_mutex_unlock(&lock[2]);
                break;
            }
        }
        //printf("cur %li %d %hhu %li\n", (clock()*1000000)/CLOCKS_PER_SEC, start, ackWaiting[start], timeouts[start]);
        //printf("spe %hhu\n", ackWaiting[5]);
        //usleep(10000);
    }
}