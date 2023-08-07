// NAME: Chathur Bommineni
// Roll Number: CS20B018
// Course: CS3205 Jan. 2023 semester
// Lab number: 5
// Date of submission: 29-4-2023
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
#include <limits.h>

#define N 64
#include <errno.h>

int	errexit(const char *format, ...);

int connectsock(const char *host, const char *service, const char *transport );
int passivesock(const char *service, const char *transport, int qlen);

//for storing deatails about the neigbouring routers
struct neighbourRouter
{
    unsigned int ID;
    unsigned int minCost;
    unsigned int maxCost;
};

//for passing arguments to update cost thread
struct costUpdate
{
    unsigned int neighID;
    unsigned int srcID;
    unsigned int cost;
};

time_t start;
unsigned int myIDval;
char* infile;
char* outfile;
FILE* fileptr;
int helloInterval;
int lsaInterval;
int spfInterval;
int numNeighbours;
struct neighbourRouter neighbours[N];

int numVertices;
unsigned int vertexSet[N];
int numEdges[N];
unsigned int adjacencyList[N][N][2];

unsigned int lsaSeq[N];

pthread_mutex_t lock[8];

void *server(void *arg);
unsigned int randCost(unsigned int srcid);
void *updateCost(void *arg);
void *helloSender(void *arg);
void *lsaSender(void *arg);
void *updateLsa(void *arg);
void *dijkstra(void *arg);

int main(int argc, char *argv[])
{
    int commands = 1;
    //read the command line arguments
    while(commands < argc)
    {
        if(strcmp(argv[commands], "-i")==0)
        {
            sscanf(argv[commands+1], "%u", &myIDval);
        }
        else if(strcmp(argv[commands], "-f")==0)
        {
            infile = argv[commands+1];
        }
        else if(strcmp(argv[commands], "-o")==0)
        {
            outfile = argv[commands+1];
        }
        else if(strcmp(argv[commands], "-h")==0)
        {
            sscanf(argv[commands+1], "%d", &helloInterval);
        }
        else if(strcmp(argv[commands], "-a")==0)
        {
            sscanf(argv[commands+1], "%d", &lsaInterval);
        }
        else if(strcmp(argv[commands], "-s")==0)
        {
            sscanf(argv[commands+1], "%d", &spfInterval);
        }
        
        commands += 2;
    }

    //initialize all the mutex locks
    for(int i=0; i<8; i++)
        if (pthread_mutex_init(&lock[i], NULL) != 0)
        {
            printf("\n mutex init has failed\n");
            return 1;
        }
    
    //initialize lsa pkt sequence numbers
    for(int i=0; i<N; i++)
        lsaSeq[i] = 0;

    //read the input file
    fileptr = fopen(infile, "r");
    if(fileptr == NULL)
    {
        printf("Infile not found.\n");
        fclose(fileptr);
        return 0;
    }

    char buffer[N];

    fgets(buffer, N, fileptr);

    int numNodes, numLinks;
    sscanf(buffer, "%d %d", &numNodes, &numLinks);
    //printf("%d %d\n", numNodes, numLinks);

    numNeighbours = 0;

    unsigned int srcNode, destNode, minc, maxc;

    //populate the information about neighbours
    while(fgets(buffer, N, fileptr) != NULL)
    {
        sscanf(buffer, "%u %u %u %u", &srcNode, &destNode, &minc, &maxc);
        if(srcNode == myIDval)
        {
            neighbours[numNeighbours].ID = destNode;
            neighbours[numNeighbours].maxCost = maxc;
            neighbours[numNeighbours].minCost = minc;
            numNeighbours++;
        }
        else if(destNode == myIDval)
        {
            neighbours[numNeighbours].ID = srcNode;
            neighbours[numNeighbours].maxCost = maxc;
            neighbours[numNeighbours].minCost = minc;
            numNeighbours++;
        }
    }

    //create/clear the output file
    fileptr = fopen(outfile, "w");
    if(fileptr == NULL)
    {
        printf("Outfile not found.\n");
        fclose(fileptr);
        return 0;
    }

    numVertices = 1;
    vertexSet[0] = myIDval;
    numEdges[0] = 0;
    
    start = time(NULL);

    //printf("%d\n", numNeighbours);

    //run all the threads
    pthread_t serverThreadid;
    pthread_create(&serverThreadid, NULL, server, NULL);

    pthread_t helloSendThreadid;
    pthread_create(&helloSendThreadid, NULL, helloSender, NULL);

    pthread_t lsaSendThreadid;
    pthread_create(&lsaSendThreadid, NULL, lsaSender, NULL);

    pthread_t dijkstraThreadid;
    pthread_create(&dijkstraThreadid, NULL, dijkstra, NULL);

    pthread_join(serverThreadid, NULL);

    return 0;
}

//handles all the incoming packets from other routers
void *server(void *arg)
{
    char buffer[1024];
    char port[N];

    sprintf(port, "%d", myIDval+10000);

    char test[11];
    char* helloReply = "HELLOREPLY";
    char* hellopkt = "HELLO";
    char* lsapkt = "LSA";

    struct sockaddr_in fsin;
    unsigned int alen;
    int socket = passivesock(port, "udp", 0);

    while(true)
    {
        alen = sizeof(fsin);
        int count = recvfrom(socket, buffer, sizeof(buffer), 0, (struct sockaddr *)&fsin, &alen);
        
        memcpy(test, buffer, 11);

        test[10] = '\0';
        if(strcmp(test, helloReply) == 0)  //helloreply packet
        {
            struct costUpdate *c = (struct costUpdate*)malloc(sizeof(struct costUpdate));
            sscanf(buffer, "HELLOREPLY %u %u %u", &(c->neighID), &(c->srcID), &(c->cost));
            //printf("Recvd Helloreply from %u %u\n", c->neighID, c->cost);
            
            //update the costs in the graph
            pthread_t costUpdateThreadID;
            pthread_create(&costUpdateThreadID, NULL, updateCost, c);
            continue;
        }

        test[5] = '\0';
        if(strcmp(test, hellopkt) == 0)  //hello packet
        {
            unsigned int srcid;
            sscanf(buffer, "HELLO %u", &srcid);
            sprintf(port, "%u", srcid + 10000);
            sprintf(buffer, "HELLOREPLY %u %u %u", myIDval, srcid, randCost(srcid));
            
            //send the helloreply
            int sock = connectsock("localhost", port, "udp");
            write(sock, buffer, strlen(buffer)+1);
            //printf("Recvd Hello from %u\n", srcid);
            continue;
        }

        test[3] = '\0';
        if(strcmp(test, lsapkt) == 0) //lsa
        {
            char *buf = (char*)malloc(strlen(buffer)+1);
            memcpy(buf, buffer, strlen(buffer)+1);
            
            //handle incoming lsa packets
            pthread_t lsaUpdateThreadid;
            pthread_create(&lsaUpdateThreadid, NULL, updateLsa, buf);
            continue;
        }
    }
}

//generate a random cost in the range of acceptable costs
unsigned int randCost(unsigned int srcid)
{
    for(int i=0; i<numNeighbours; i++)
    {
        if(neighbours[i].ID == srcid)
        {
            return neighbours[i].minCost + rand()%(neighbours[i].maxCost - neighbours[i].minCost);
        }
    }
}

//update the cost of a link in the graph
void *updateCost(void *arg)
{
    struct costUpdate *c = arg;

    //printf("update %u %u %u\n", c->srcID, c->neighID, c->cost);

    pthread_mutex_lock(&lock[0]);
    
    for(int i=0; i<numVertices; i++)
    {
        if(vertexSet[i] == c->srcID)
        {
            for(int j=0; j<=numEdges[i]; j++)
            {
                if(j == numEdges[i])
                {
                    adjacencyList[i][numEdges[i]][0] = c->neighID;
                    adjacencyList[i][numEdges[i]][1] = c->cost;
                    numEdges[i]++;
                    break;
                }
                if(adjacencyList[i][j][0] == c->neighID)
                {
                    adjacencyList[i][j][1] = c->cost;
                    break;
                }
            }
            break;
        }
        if(i == numVertices-1)
        {
            vertexSet[numVertices] = c->srcID;
            numEdges[numVertices] = 1;
            adjacencyList[numVertices][0][0] = c->neighID;
            adjacencyList[numVertices][0][1] = c->cost;
            numVertices++;
        }
    }
    for(int i=0; i<numVertices; i++)
    {
        if(vertexSet[i] == c->neighID)
        {
            for(int j=0; j<numEdges[i]; j++)
            {
                if(j == numEdges[i])
                {
                    adjacencyList[i][numEdges[i]][0] = c->srcID;
                    adjacencyList[i][numEdges[i]][1] = c->cost;
                    numEdges[i]++;
                    break;
                }
                if(adjacencyList[i][j][0] == c->srcID)
                {
                    adjacencyList[i][j][1] = c->cost;
                    break;
                }
            }
            break;
        }
        if(i == numVertices-1)
        {
            vertexSet[numVertices] = c->neighID;
            numEdges[numVertices] = 1;
            adjacencyList[numVertices][0][0] = c->srcID;
            adjacencyList[numVertices][0][1] = c->cost;
            numVertices++;
        }
    }

    pthread_mutex_unlock(&lock[0]);

    /*
    for(int i=0; i<numVertices; i++)
    {
        printf("%u ", vertexSet[i]);
        for(int j=0; j<numEdges[i]; j++)
            printf("%u-%u ", adjacencyList[i][j][0], adjacencyList[i][j][1]);
        printf("\n");
    }
    */
    
    free(c);
    
    return NULL;
}

//thread for sending hello packets at a regular interval
void *helloSender(void *arg)
{
    int socket[numNeighbours];

    char port[N];
    for(int i=0; i<numNeighbours; i++)
    {
        sprintf(port, "%u", neighbours[i].ID+10000);
        socket[i] = connectsock("localhost", port, "udp");
    }

    char buffer[N];
    sprintf(buffer, "HELLO %u", myIDval);

    while(true)
    {
        //send hello packets to each of the neighbours
        for(int i=0; i<numNeighbours; i++)
        {
            write(socket[i], buffer, strlen(buffer)+1);
            //printf("Sent Hello to %u\n", neighbours[i].ID);
        }
        sleep(helloInterval);
    }
}

//thread for sending lsa packets at a regular intervals
void *lsaSender(void *arg)
{
    int socket[numNeighbours];
    char port[N];
    for(int i=0; i<numNeighbours; i++)
    {
        sprintf(port, "%u", neighbours[i].ID+10000);
        socket[i] = connectsock("localhost", port, "udp");
    }

    char buffer[N];
    int seq = 1;

    while(true)
    {
        sleep(lsaInterval);
        pthread_mutex_lock(&lock[0]);
        pthread_mutex_lock(&lock[1]);

        sprintf(buffer, "LSA %u %u %u", myIDval, seq, numNeighbours);
        for(int i=0; i<numNeighbours; i++)
        {
            sprintf(buffer+strlen(buffer), " %u %u", adjacencyList[0][i][0], adjacencyList[0][i][1]);
        }

        //send hello packets to each of the neighbours
        for(int i=0; i<numNeighbours; i++)
        {
            write(socket[i], buffer, strlen(buffer)+1);
        }
        //printf("Sent LSA pkt %u\n", seq);
        seq++;
        lsaSeq[myIDval] = seq;

        pthread_mutex_unlock(&lock[1]);
        pthread_mutex_unlock(&lock[0]);
    }
}

//handle incoming lsa packets
void *updateLsa(void *arg)
{
    char* buf = arg;
    char copy[N];

    pthread_mutex_lock(&lock[1]);

    bool update = false;
    unsigned int srcid;
    unsigned int seq;
    sscanf(buf, "LSA %u %u ", &srcid, &seq);
    sprintf(copy, "LSA %u %u ", srcid, seq);
    //printf("Recvd lsa pkt %u %u\n", srcid, seq);

    //check if the sequence number was already recieved
    if(seq > lsaSeq[srcid])
    {
        //printf("%s\n", buf);
        update = true;
        int pos = strlen(copy);
        unsigned int num;
        sscanf(buf+pos, "%u", &num);
        sprintf(copy, "%u", num);
        pos += strlen(copy);

        for(unsigned int i=0; i<num; i++)
        {
            //do the cost updates for the links in the lsa packet
            struct costUpdate* c = (struct costUpdate*)malloc(sizeof(struct costUpdate));
            c->srcID = srcid;
            sscanf(buf+pos, " %u %u", &(c->neighID), &(c->cost));
            //printf("pos %d lsa %u %u %u\n", pos, c->neighID, c->srcID, c->cost);
            sprintf(copy, " %u %u", c->neighID, c->cost);
            pos += strlen(copy);
            pthread_t costUpdateThreadid;
            pthread_create(&costUpdateThreadid, NULL, updateCost, c);
        }

        lsaSeq[srcid] = seq;
    }

    pthread_mutex_unlock(&lock[1]);

    //if it is a new packet then broadcast to neighbours
    if(update)
    {
        char port[N];
        for(int i=0; i<numNeighbours; i++)
        {
            sprintf(port, "%u", neighbours[i].ID+10000);
            int socket = connectsock("localhost", port, "udp");
            write(socket, buf, strlen(buf)+1);
        }
        //printf("Resend lsa pkt from %u %u\n", srcid, seq);
    }

    free(buf);
}

//run dijkstra's algorithm and generate the routing table at regular intervals
void *dijkstra(void *arg)
{
    while(true)
    {
        sleep(spfInterval);

        pthread_mutex_lock(&lock[0]);

        //initialize the variables for all nodes
        unsigned int distances[numVertices];
        bool update[numVertices];
        int numHops[N];
        unsigned int route[N][N];

        for(int i=0; i<numVertices; i++)
        {
            distances[i] = UINT_MAX;
            update[i] = false;
            numHops[i] = -1;
            route[i][0] = i;
        }
        
        distances[myIDval] = 0;
        update[myIDval] = true;
        numHops[myIDval] = 1;
        route[myIDval][0] = myIDval;

        for(int i=0; i<numVertices; i++)
        {
            numHops[i] = 1;
            route[i][0] = myIDval;
        }

        //run the dijkstra's algorithm
        for(int i=0; i<numVertices; i++)
        {
            unsigned int cur = vertexSet[i];
            if(update[cur])
            {
                update[cur] = false;
                //printf("%u\n", cur);
                for(int j=0; j<numEdges[i]; j++)
                {
                    unsigned int neigh = adjacencyList[i][j][0];
                    //update distances if necessary
                    if(distances[neigh] > distances[cur] + adjacencyList[i][j][1])
                    {
                        distances[neigh] = distances[cur] + adjacencyList[i][j][1];
                        for(int k=0; k<numHops[cur]; k++)
                        {
                            route[neigh][k] = route[cur][k];
                        }
                        route[neigh][numHops[cur]] = neigh;
                        numHops[neigh] = numHops[cur] + 1;
                        update[neigh] = true;
                    }
                }
                i = 0;
            }
        }

        //print the current graph and routing table
        printf("graph\n");
        for(int i=0; i<numVertices; i++)
        {
            printf("%u ", vertexSet[i]);
            for(int j=0; j<numEdges[i]; j++)
                printf("%u-%u ", adjacencyList[i][j][0], adjacencyList[i][j][1]);
            printf("\n");
        }
        printf("\nroutes\n");
        for(int i=0; i<numVertices; i++)
        {
            for(int j=0; j<numHops[i]; j++)
                printf("%u-", route[i][j]);
            printf(" %u\n", distances[i]);
        }
        printf("\n");
        
        pthread_mutex_unlock(&lock[0]);

        fileptr = fopen(outfile, "a");

        char buf[N];
        int prev;
        int next;
        
        //print the routing table to the output file
        fprintf(fileptr, "Routing Table for Node No. %u at time %lu:\n", myIDval, time(NULL)-start);
        fprintf(fileptr, "|  Destination  |       Path       |  Cost  |\n");
        for(int i=0; i<numVertices; i++)
        {
            unsigned int cur = vertexSet[i];
            if(cur == myIDval)
                continue;
            
            sprintf(buf, "%u", cur);
            prev = (15 - strlen(buf))/2;
            next = 15 - strlen(buf) - prev;

            fprintf(fileptr, "|");
            for(int j=0; j<prev; j++)
                fprintf(fileptr, " ");
            fprintf(fileptr, "%u", cur);
            for(int j=0; j<next; j++)
                fprintf(fileptr, " ");
            
            sprintf(buf, "%u", route[cur][0]);
            for(int j=1; j<numHops[cur]; j++)
                sprintf(buf+strlen(buf), "-%u", route[cur][j]);
            prev = (18 - strlen(buf))/2;
            next = 18 - strlen(buf) - prev;
            
            fprintf(fileptr, "|");
            for(int j=0; j<prev; j++)
                fprintf(fileptr, " ");
            fprintf(fileptr, "%s", buf);
            for(int j=0; j<next; j++)
                fprintf(fileptr, " ");
            
            sprintf(buf, "%u", distances[cur]);
            prev = (8 - strlen(buf))/2;
            next = 8 - strlen(buf) - prev;

            fprintf(fileptr, "|");
            for(int j=0; j<prev; j++)
                fprintf(fileptr, " ");
            fprintf(fileptr, "%u", distances[cur]);
            for(int j=0; j<next; j++)
                fprintf(fileptr, " ");
            fprintf(fileptr, "|\n");
        }
        fprintf(fileptr, "\n");

        fclose(fileptr);
    }
}