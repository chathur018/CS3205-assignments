#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#define N 256

//struct for storing the input bit-string
struct tripleInt
{
    uint64_t high, mid, low;
};

//constructor for triple int
//takes the input char-string and turns it into a bit-string
void tripleConstuctor(struct tripleInt* message, char* input)
{
    message->high = message->mid = message->low = 0;

    //first 8 bits
    for(int i=0; i<8; i++)
    {
        message->high *= 2;
        if(input[i] == '1')
            message->high++;
    }
    //next 64 bits
    for(int i=8; i<72; i++)
    {
        message->mid *= 2;
        if(input[i] == '1')
            message->mid++;
    }
    //remaining bits
    for(int i=72; i<128; i++)
    {
        message->low *= 2;
        if(input[i] == '1')
            message->low++;
    }
    message->low = message->low << 8;

    return;
}

//computes the crc8 of the given message
uint64_t crc8(struct tripleInt message)
{
    uint64_t div = 0;

    //generating the polynomial bit-string from a char-string
    char* divcode = "100000111";
    for(int i=0; i<strlen(divcode); i++)
    {
        div *= 2;
        if(divcode[i] == '1')
            div++;
    }

    //printf("%lu %lu %lu\n", message.high, message.mid, message.low);

    struct tripleInt dvd;  //stores the quotient
    dvd.high = dvd.mid = dvd.low = 0;

    //start operation from leftmost point
    uint64_t temp = div << 55;
    uint64_t pos = (uint64_t)1 << 63;
    //printf("%lu\n", temp);
    //doing the operation on the first 8 bits
    for(int i=0; i<64; i++)
    {
        dvd.high *= 2;

        if(pos & message.high)
        {
            dvd.high++;
            message.high = message.high ^ temp;
            //printf("a ");

            //if the operation is happening at the border
            if(i >= 56)
            {
                message.mid = message.mid ^ (div << (55+64-i));
            }
        }        

        //move to next bit
        temp = temp >> 1;
        pos = pos >> 1;
    }
    //printf("\n");

    //printf("%lu %lu %lu\n", dvd.high, message.mid, message.low);

    temp = div << 55;
    pos = (uint64_t)1 << 63;
    //printf("%lu\n", temp);
    //doing operation for next 64 bits
    for(int i=0; i<64; i++)
    {
        dvd.mid *= 2;

        if(pos & message.mid)
        {
            dvd.mid++;
            message.mid = message.mid ^ temp;
            //printf("a ");
            //if the operation is happening at the border
            if(i >= 56)
            {
                message.low = message.low ^ (div << (55+64-i));
            }
        }
        
        //move to next bit
        temp = temp >> 1;
        pos = pos >> 1;
    }
    //printf("\n");

    //printf("%lu %lu %lu\n", dvd.high, dvd.mid, message.low);

    temp = div << 55;
    pos = (uint64_t)1 << 63;
    //printf("%lu\n", temp);
    //doing operation on the last 56 bits
    for(int i=0; i<56; i++)
    {
        dvd.low *= 2;

        if(pos & message.low)
        {
            dvd.low++;
            message.low = message.low ^ temp;
            //printf("a ");
        }

        //move to next bit
        temp = temp >> 1;
        pos = pos >> 1;
    }
    //printf("\n");

    //printf("%lu %lu %lu\n", dvd.high, dvd.mid, dvd.low);
    
    return message.low;
}

//corrupt a message with num no. of errors
struct tripleInt tripleCorrupt(struct tripleInt message, unsigned int num)
{
    int pos[num];
    //generate num no. of positions to put errors in
    for(int i=0; i<num; i++)
    {
        while(1)
        {
            int temp = rand()%136;
            //if it matches with a previous position, then discard
            for(int j=0; j<i; j++)
                if(pos[j] == temp)
                    continue;
            pos[i] = temp;
            break;
        }
        //printf("%d ", pos[i]);
    }
    //printf("\n");

    //put the errors into the message
    for(int i=0; i<num; i++)
        if(pos[i] < 64)
            message.low = message.low ^ ((uint64_t)1 << pos[i]);
        else if (pos[i] < 128)
            message.mid = message.mid ^ ((uint64_t)1 << (pos[i]-64));
        else
            message.high = message.high ^ ((uint64_t)1 << (pos[i]-128));
    
    return message;
}

//corrut a message with a burst error of length 6 at positon pos
struct tripleInt tripleCorruptBurst(struct tripleInt message, unsigned int pos)
{
    message.low = message.low ^ ((uint64_t)63 << (31-pos));
    
    return message;
}

//convert a bit-string to char-string and put in str
void tripleToString(struct tripleInt message, char* str)
{
    //printf("a");

    //top 8 bits
    uint64_t pos = (uint64_t)1 << 7;
    int i = 0;
    while(pos)
    {
        //printf("%lu", pos);
        if(message.high & pos)
            str[i] = '1';
        else
            str[i] = '0';
        i++;
        //next bit
        pos = pos >> 1;
    }
    //next 64 bits
    pos = (uint64_t)1 << 63;
    while(pos)
    {
        if(message.mid & pos)
            str[i] = '1';
        else
            str[i] = '0';
        i++;
        //next bit
        pos = pos >> 1;
    }
    //last 64 bits
    pos = (uint64_t)1 << 63;
    while(pos)
    {
        if(message.low & pos)
            str[i] = '1';
        else
            str[i] = '0';
        i++;
        //next bit
        pos = pos >> 1;
    }
    str[i] = '\0';
    //printf("%d", i);

    return;
}

int main(int argc, char* argv[])
{
    //check for proper usage
    if(argc < 3)
    {
        printf("usage: ec [infile] [outfile]\n");
        return 0;
    }

    //open and clear the output file
    FILE* fileptr = fopen(argv[2], "w");
    if(fileptr == NULL)
    {
        printf("Output file not found.\n");
        fclose(fileptr);
        return 0;
    }
    
    //open and read the input file
    fileptr = fopen(argv[1], "r");
    if(fileptr == NULL)
    {
        printf("Input file not found.\n");
        fclose(fileptr);
        return 0;
    }

    char buffer[N];
    char corrupted[N];
    srand(time(0));

    //read and process each line
    while(fgets(buffer, N, fileptr) != NULL)
    {
        //end the string
        buffer[128] = '\0';
        //printf("%s\n", buffer);

        //create empty message
        struct tripleInt message;
        //populate message
        tripleConstuctor(&message, buffer);
        //run crc8
        uint64_t rem = crc8(message);
        //printf("%lu\n", rem);

        //append the check sum to the message
        struct tripleInt crc = message;
        crc.low = message.low | rem;

        char remainder[9];
        remainder[8] = '\0';
        for(int i=0; i<9; i++)
        {
            if(rem%2 == 1)
                remainder[7-i] = '1';
            else
                remainder[7-i] = '0';
            rem /= 2;
        }
        //printf("%s\n", remainder);

        //open output file to append
        FILE* outfile = fopen(argv[2], "a");
        if(fileptr == NULL)
        {
            printf("Output file not found.\n");
            fclose(fileptr);
            exit(0);
        }

        fprintf(outfile, "Original String          : %s\n", buffer);
        fprintf(outfile, "Original String with CRC : %s%s\n", buffer, remainder);

        //rem = crc8(crc);
        //printf("%lu\n", rem);

        struct tripleInt corrupt;
        //corrupt = tripleCorrupt(message, 3);
        //tripleToString(corrupt, corrupted);
        //printf("%s\n", corrupted);

        //single bit errors
        for(int i=0; i<10; i++)
        {
            unsigned int temp = (rand()%67)*2+3;
            //printf("%u\n", temp);
            corrupt = tripleCorrupt(crc, temp);
            tripleToString(corrupt, corrupted);
            fprintf(outfile, "Corrupted String         : %s\n", corrupted);
            fprintf(outfile, "Number of Errors         : %d\n", temp);
            rem = crc8(corrupt);
            if(rem)
                fprintf(outfile, "CRC Check : Failed (Corruption is detected)\n");
            else
                fprintf(outfile, "CRC Check : Passed (Corruption is not detected)\n");
        }

        //fprintf(outfile, "Original String          : %s\n", buffer);
        //fprintf(outfile, "Original String with CRC : %s%s\n", buffer, remainder);
        
        //burst errors
        for(int i=0; i<5; i++)
        {
            unsigned int temp = rand()%11;
            //printf("%u\n", temp);
            corrupt = tripleCorruptBurst(crc, temp);
            tripleToString(corrupt, corrupted);
            fprintf(outfile, "Corrupted String         : %s\n", corrupted);
            fprintf(outfile, "Position of Burst Error  : %d\n", temp+100);
            rem = crc8(corrupt);
            if(rem)
                fprintf(outfile, "CRC Check : Failed (Corruption is detected)\n");
            else
                fprintf(outfile, "CRC Check : Passed (Corruption is not detected)\n");
        }

        fprintf(outfile, "\n");

        fclose(outfile);
    }

    fclose(fileptr);
    return 0;
}
