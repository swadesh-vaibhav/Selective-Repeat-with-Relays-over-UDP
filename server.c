#include <stdio.h>
#include "packet.h"

//For displaying errors and exiting if needed

int data_received[WINSIZE];     //0 if not received, 1 if received
char window[WINSIZE][PACKET_SIZE];              //buffer array
int lastreceived;                   //which packet is the last packet
int fin;                            //whether all packets have been received or not. 1 to end the program.
int lastsize;                       //size of last packet

void copyString(char * dest, char * src)
{
    for(int i=0; i<PACKET_SIZE; i++)
    {
        dest[i] = src[i];
    }
}

void writetofile(FILE* fp, packet current)
{
    int seq = (current.seq/PACKET_SIZE)%WINSIZE;
    data_received[seq]=1;

    //find and insert in the correct buffer array element, according to correct sequence number
    // sprintf(window[seq], "%s", current.payload);    
    copyString(window[seq], current.payload);
    if(current.last==1)
    {
        lastreceived = seq;
        lastsize = current.size;
    }

    fin = 0;   //0 to move window forward, 1 to wait for more packets, 2 to write and end the program

    for(int i = 0; i<WINSIZE; i++)
    {
        if(i==lastreceived)
        {
            //all previous packets have been received and last has also been received
            fin = 2;
            break;
        }
        if(data_received[i]==0)
        {
            //one of the packets has not been received
            fin = 1;
            break;
        }
    }

    if(fin == 0)
    {
        // printf("Entered\n");
        //move window forward
        for(int i =0; i<WINSIZE; i++)
        {
            // printf("%s", window[i]);
            fwrite(window[i], 1, PACKET_SIZE, fp);
            fflush(fp);
            data_received[i]=0;
        }
        // printf("Exited\n");
    }
    else if(fin == 2)
    {
        //write till the last packet. All have been received
        for(int i =0; i<lastreceived; i++)
        {
            fwrite(window[i], 1, PACKET_SIZE, fp);
            fflush(fp);
            data_received[i]=0;
        }
        fwrite(window[lastreceived], 1, lastsize, fp);
        fflush(fp);
        data_received[lastreceived]=0;
    }
}

void printerror(char* error, int toclose)
{
	perror(error);
	if(toclose==1)
		exit(0);
}

//Trace

void trace(packet pkt)
{
	if(pkt.packettype == data)
	{
		printf("\nRCVD PACKET: Sequence Number %d of Size %d Bytes from Relay %d\n", pkt.seq, pkt.size, (pkt.channel==0?2:1));
	}
	else
	{
		printf("\nSENT ACK for packet with Sequence Number %d of Size %d Bytes from Relay %d \n", pkt.seq, pkt.size, (pkt.channel==0?2:1));
	}
}

int main(void)
{

	//Socket Descriptors

	int fd;
    int fd1;
    int fd2;

	//Declarations of essentials

	packet data1;
	packet ack1;

	int tempsize;
	int tempseq;
	int templast;

	char buffer[PACKET_SIZE];

	struct sockaddr_in server, client;
    // memset(&client, 0, sizeof(client));
	fd_set rdfds;
	struct timeval tv;

    int opt;

	//Initialization

	templast = 0;
	FD_ZERO(&rdfds);
	tv.tv_sec = TIMEOUT;
	tv.tv_usec = 0;
    opt = 1;
    lastreceived = -1;
    fin = 0;        //1 if finished sending all packets

    memset(buffer, 0, sizeof(buffer));
    memset(&server, 0, sizeof(server));
    memset(&client, 0, sizeof(client));
    for(int i=0; i<WINSIZE;i++)
    {
        data_received[i]=0;
        // window[i] = (char *)malloc(WINSIZE*sizeof(char));
    }

    // srand(time(0));

	//Sockets

	if((fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP))<0)
		printerror("creating socket", 1);

	//Server Address

	server.sin_family = AF_INET;
	server.sin_port = htons(12346);
	server.sin_addr.s_addr = inet_addr("127.0.0.1");
	
    //Binding

    if((bind(fd, (struct sockaddr*)&server, sizeof(server)))<0)
    {
        printerror("binding\n", 1);
    }
    //Allow multiple connections

	// FD_SET(fd1, &rdfds);
    // FD_SET(fd2, &rdfds);

	//Opening file to be read

	FILE *fp;
	fp = fopen(DEST, "w");

	while(1)
	{
		//Now, both the channels are waiting for data
        
	    // FD_ZERO(&rdfds);
        // FD_SET(fd1, &rdfds);
        // FD_SET(fd2, &rdfds);

		// int retval = select(10, &rdfds, NULL, NULL, &tv);
		// tv.tv_sec = TIMEOUT;
		// tv.tv_usec = 0;

		// if(retval==-1)
		// {
		// 	printerror("selecting \n", 1);
		// }
		// else if(retval)
		// {
			//One of the channels has received an data

			// if(FD_ISSET(fd1, &rdfds))
			// {
				//First channel has received the data
                int len = sizeof(client);
				int bytesRecvd = recvfrom(fd, &data1, sizeof(data1), 0, (struct sockaddr*)&client, &len);
                //Implement Packet loss

                trace(data1);

                if(bytesRecvd<0)
                {
                    printerror("receiving at channel 1\n", 0);
                }
                else
                {
                    writetofile(fp, data1);

                    //Create ack      
                    ack1 = data1;
                    ack1.packettype = ak;
    
                    //Send ack
        
                    while((sendto(fd, &ack1, sizeof(ack1), 0, (struct sockaddr*)&client, sizeof(client)))<0)
                    {
                        // printerror("sending packet from first channel\n", 0);
                    }
                    trace(ack1);
                }
	}
	// close(fd);
}