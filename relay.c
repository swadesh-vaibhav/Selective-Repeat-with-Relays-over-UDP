#include "packet.h"
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>
int main(int argc, char * argv[])
{
    if(argc!=2)
    {
        printf("You should also enter 1/2 (to start a corresponding relay node)");
        exit(0);
    }

    int fd, recvLen, drop, signal;
    struct sockaddr_in clientAddr, serverAddr, relayAddr, recvAddr;
    int whichRelay = atoi(argv[1]);

    fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if(fd == -1)
    {
        printf("Error in opening socket()\n");
        exit(0);
    }

    memset(&clientAddr, '\0', sizeof(clientAddr));
    clientAddr.sin_family = AF_INET;
    clientAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    clientAddr.sin_port = htons(12345);

    memset(&serverAddr, '\0', sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverAddr.sin_port = htons(12346);

    memset(&relayAddr, '\0', sizeof(relayAddr));
    relayAddr.sin_family = AF_INET;
    relayAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    if(whichRelay == 1)
        relayAddr.sin_port = htons(12347);
    else
        relayAddr.sin_port = htons(12348);

    signal = bind(fd, (struct sockaddr *)&relayAddr, sizeof(relayAddr));

    if(signal == -1)
    {
        printf("Error in binding the socket() to the relay addr\n");
        exit(0);
    }

    packet pktRcvd, pktToSend;
    char timer[8];
    memset(timer, '\0', sizeof(char)*8);
    srand(time(0));

    while(1)
    {
        if(recvfrom(fd, &pktRcvd, sizeof(pktRcvd), 0, (struct sockaddr *)&recvAddr, &recvLen) < 0)
        {
            printf("Error in rcvfrom() in relay\n");
            exit(0);
        }

        drop = rand()%100;

        //time = getTime()
        if(drop < PERCENT_LOSS)
        {
            if(whichRelay == 1)
            {
                if(pktRcvd.packettype == 1)
                    printf("%10s %10s %10s %10s %10d %10s %10s\n", "RELAY 1", "D", timer, "DATA", pktRcvd.seq, "CLIENT", "RELAY 1");
                else
                    printf("%10s %10s %10s %10s %10d %10s %10s\n", "RELAY 1", "D", timer, "ACK", pktRcvd.seq, "SERVER", "RELAY 1");
            }
            else
            {
                if(pktRcvd.packettype == 1)
                    printf("%10s %10s %20s %10s %10d %10s %10s\n", "RELAY 2", "D", timer, "DATA", pktRcvd.seq, "CLIENT", "RELAY 2");
                else
                    printf("%10s %10s %20s %10s %10d %10s %10s\n", "RELAY 2", "D", timer, "ACK", pktRcvd.seq, "SERVER", "RELAY 2");
            }
            continue;
        }

        if(whichRelay == 1)
        {
            if(pktRcvd.packettype == 1)
                printf("%10s %10s %10s %10s %10d %10s %10s\n", "RELAY 1", "R", timer, "DATA", pktRcvd.seq, "CLIENT", "RELAY 1");
            else
                printf("%10s %10s %10s %10s %10d %10s %10s\n", "RELAY 1", "R", timer, "ACK", pktRcvd.seq, "SERVER", "RELAY 1");
        }
        else
        {
            if(pktRcvd.packettype == 1)
                printf("%10s %10s %20s %10s %10d %10s %10s\n", "RELAY 2", "R", timer, "DATA", pktRcvd.seq, "CLIENT", "RELAY 2");
            else
                printf("%10s %10s %20s %10s %10d %10s %10s\n", "RELAY 2", "R", timer, "ACK", pktRcvd.seq, "SERVER", "RELAY 2");
        }

        usleep(rand()%3);

        if(pktRcvd.packettype == 1)
            signal = sendto(fd, &pktRcvd, sizeof(packet), 0, (struct sockaddr*)&serverAddr, sizeof(struct sockaddr_in));
        else
            signal = sendto(fd, &pktRcvd, sizeof(packet), 0, (struct sockaddr*)&clientAddr, sizeof(struct sockaddr_in));
        
        if(signal != sizeof(packet))
        {
            perror("Error\n");
            printf("Error in sendto()\n");
            exit(0);
        }

        //time = getTime()
        if(whichRelay == 1)
        {
            if(pktRcvd.packettype == 1)
                printf("%10s %10s %10s %10s %10d %10s %10s\n", "RELAY 1", "S", timer, "DATA", pktRcvd.seq, "RELAY 1", "SERVER");
            else
                printf("%10s %10s %10s %10s %10d %10s %10s\n", "RELAY 1", "S", timer, "ACK", pktRcvd.seq, "RELAY 1", "CLIENT");
        }
        else
        {
            if(pktRcvd.packettype == 1)
                printf("%10s %10s %20s %10s %10d %10s %10s\n", "RELAY 2", "S", timer, "DATA", pktRcvd.seq, "RELAY 2", "SERVER");
            else
                printf("%10s %10s %20s %10s %10d %10s %10s\n", "RELAY 2", "S", timer, "ACK", pktRcvd.seq, "RELAY 2", "CLIENT");
        }
    }
}