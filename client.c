#include <stdio.h>
#include "packet.h"

//For displaying errors and exiting if needed

void printerror(char* error, int toclose)
{
	//second argument 1 to kill
	// printf("Error while %s\n", error);
	perror(error);
	if(toclose==1)
		exit(0);
}
void copyString(char * dest, char * src)
{
    for(int i=0; i<PACKET_SIZE; i++)
    {
        dest[i] = src[i];
    }
}
//Trace

void trace(packet pkt)
{
	if(pkt.packettype == data)
	{
		printf("\nSENT PACKET: Sequence Number %d of Size %d Bytes to Relay %d\n", pkt.seq, pkt.size, (pkt.channel==0?2:1));
	}
	else
	{
		printf("\nRCVD ACK for packet with Sequence Number %d of Size %d Bytes from Relay %d \n", pkt.seq, pkt.size, (pkt.channel==0?2:1));
	}
}


int main(void)
{

	//Socket Descriptors

	int fd1;

	//Declarations of essentials

	packet data1;		//to store packets
	packet ack1;
	packet window[WINSIZE];
	
	int lastackreceived;	//whether ack for all files has been received

	int tempsize;		//to store attributes
	int tempseq;
	int templast;

	char buffer[PACKET_SIZE];	//buffer for message passing

	struct sockaddr_in relay1, client;	//relay1 address
	struct sockaddr_in relay2, dummy;

	client.sin_family = AF_INET;
	client.sin_port = htons(12345);
	client.sin_addr.s_addr = inet_addr("127.0.0.1");
	fd_set rdfds;				//fdset for select
	struct timeval tv;			//timeval for timer

	int ack_received[WINSIZE];		//0 if ack not received, 1 if received, 2 if packet not even sent(packet does not exist)

	//Initialization

	templast = 0;
	FD_ZERO(&rdfds);
	tv.tv_sec = TIMEOUT;
	tv.tv_usec = 0;
	lastackreceived = 0;
	tempsize = 0;
	for(int i =0; i<WINSIZE; i++)
	{
		ack_received[i] = 2;
	}

	//Sockets

	if((fd1 = socket(AF_INET, SOCK_DGRAM, 0))<0)
		printerror("creating first socket", 1);
	// if((fd2 = socket(AF_INET, SOCK_DGRAM, 0))<0)
	// 	printerror("creating second socket", 1);
	
	if(bind(fd1, (struct sockaddr*)&client, sizeof(client)) < 0)
		printerror("binding client socket", 1);
	
	//Zeroing out memories
	memset(&relay1, 0, sizeof(relay1));
	memset(&relay2, 0, sizeof(relay2));
	memset(buffer, 0, sizeof(buffer));

	//Relay Address

	relay1.sin_family = AF_INET;
	relay1.sin_port = htons(12347);
	relay1.sin_addr.s_addr = inet_addr("127.0.0.1");
	
	relay2.sin_family = AF_INET;
	relay2.sin_port = htons(12348);
	relay2.sin_addr.s_addr = inet_addr("127.0.0.1");
	
	//Connecting to server

	// if((connect(fd1, (struct sockaddr*) &relay1, sizeof(relay1)))<0)
	// 	printerror("connecting first socket", 1);
	// if((connect(fd2, (struct sockaddr*) &relay2, sizeof(relay2)))<0)
	// 	printerror("connecting second socket", 1);
	
	FD_SET(fd1, &rdfds);
	// FD_SET(fd1, &rdfds);

	//Opening file to be read

	FILE *fp;
	fp = fopen(SOURCE, "r");

	//Creating first window

	for(int i = 0 ; i<WINSIZE; i++)
	{
		tempseq = ftell(fp);
		tempsize = fread(buffer, 1, PACKET_SIZE, fp);
		if(feof(fp))
		{
			templast = 1;
		}

		// sprintf(data1.payload, "%s", buffer);
		copyString(data1.payload, buffer);
		// data1.payload[tempsize]='\0';
		data1.size = tempsize;
		data1.seq = tempseq;
		data1.last = templast;
		data1.packettype = data;
		data1.channel = (data1.seq/PACKET_SIZE)%2;

		window[i] = data1;
		ack_received[i] = 0;

		if(feof(fp))
			break;
	}

	//Send first window

	for(int i =0; i<WINSIZE; i++)
	{
		data1 = window[i];
		if(ack_received[i]==0)
		{
			if(data1.channel==0)
			{
				// while((write(fd2, (const void *) &data1, sizeof(data1)))<0);
				while((sendto(fd1, &data1, sizeof(data1), 0 , (struct sockaddr*) &relay2, sizeof(relay2)))<0);
				{
					// printerror("sending packet from first channel\n", 0);
				}
				trace(data1);
			}
			else
			{
				
				while((sendto(fd1, &data1, sizeof(data1), 0 , (struct sockaddr*) &relay1, sizeof(relay1)))<0);
				{
					// printerror("sending packet from first channel\n", 0);
				}
				trace(data1);
			}
		}
	}

	int bytesRecvd;

	while(!lastackreceived)
	{
		//Add fds to monitor and reset timer

		FD_ZERO(&rdfds);
		FD_SET(fd1, &rdfds);
		// FD_SET(fd2, &rdfds);
		tv.tv_sec = TIMEOUT;
		tv.tv_usec = MTIMEOUT;

		//Now, both the channels are waiting for acks

		int retval = select(10, &rdfds, NULL, NULL, &tv);

		if(retval==-1)
		{
			printerror("select\n", 1);
		}
		else if(retval)
		{
			//One of the channels has received an ack

			if(FD_ISSET(fd1, &rdfds))
			{
				// First channel has received the ack
				int len = sizeof(struct sockaddr);

				while((bytesRecvd = recvfrom(fd1, &ack1, sizeof(ack1), 0, (struct sockaddr*)&dummy, &len))==0);
				if(bytesRecvd<0)
				{
					printerror("receiving at channel 1\n", 0);
				}
				else
				{
					if(ack_received[((ack1.seq)/(PACKET_SIZE))%WINSIZE]==0&&window[((ack1.seq)/(PACKET_SIZE))%WINSIZE].seq==ack1.seq)
					{
						// printf("Match found");
						ack_received[((ack1.seq)/(PACKET_SIZE))%WINSIZE]=1;
						trace(ack1);
						if(ack1.last==1)
						{
							lastackreceived = 1;
							for(int i = 0; i<((ack1.seq)/(PACKET_SIZE))%WINSIZE; i++)
							{
								if(ack_received[i]==1)
									;
								else
									lastackreceived = 0;
							}
						}
					
						int flag = 0;	//0 to move window forward, 1 to send packets, 2 to stop the client

						for(int i =0; i<WINSIZE; i++)
						{
							if(ack_received[i]==0)
							{
								flag = 1;
								break;
							}
							if(ack_received[i]==2)
							{
								//packet not read, and all previous packets have received their acks
								flag = 2;
								break;
							}
						}
						
						if(flag == 0)
						{
							//move window forward
							//set all packets to not yet read
							for(int i = 0; i<WINSIZE; i++)
							{
								ack_received[i]=2;
							}

							//read packets from file
							for(int i = 0 ; i<WINSIZE; i++)
							{
								tempseq = ftell(fp);
								tempsize = fread(buffer, 1, PACKET_SIZE, fp);
								if(feof(fp))
								{
									templast = 1;
								}

								// sprintf(data1.payload, "%s", buffer);
								copyString(data1.payload, buffer);
								data1.size = tempsize;
								data1.seq = tempseq;
								data1.last = templast;
								data1.packettype = data;
								data1.channel = (data1.seq/WINSIZE)%2;

								window[i] = data1;
								ack_received[i] = 0;

								if(templast)
									break;
							}

							//send packets of new window

							for(int i =0; i<WINSIZE; i++)
							{
								data1 = window[i];
								if(ack_received[i]==0)
								{
									if(data1.channel==0)
									{
										while((sendto(fd1, &data1, sizeof(data1), 0 , (struct sockaddr*) &relay2, sizeof(relay2)))<0);
										{
											// printerror("sending packet from first channel\n", 0);
										}
										trace(data1);
									}
									else
									{
										while((sendto(fd1, &data1, sizeof(data1), 0 , (struct sockaddr*) &relay1, sizeof(relay1)))<0);
										{
											// printerror("sending packet from second channel\n", 0);
										}
										trace(data1);
									}
								}
							}
						}
						else if(flag==2)
						{
							//exit
							break;
						}
					}
				}
					//else send the required packets after timer ends
			}
			// if(FD_ISSET(fd2, &rdfds))
			// {
			// 	//Second channel has received the ack
			// 	while((bytesRecvd = recvfrom(fd2, &ack1, sizeof(ack1), 0, (struct sockaddr*)dummy, dummy))==0);
			// 	if(bytesRecvd<0)
			// 	{
			// 		printerror("receiving at channel 1\n", 0);
			// 	}
			// 	else
			// 	{
			// 		if(ack_received[((ack1.seq)/(PACKET_SIZE))%WINSIZE]==0&&window[((ack1.seq)/(PACKET_SIZE))%WINSIZE].seq==ack1.seq)
			// 		{
			// 			// printf("Match found");
			// 			ack_received[((ack1.seq)/(PACKET_SIZE))%WINSIZE]=1;
			// 			trace(ack1);
					
			// 			int flag = 0;	//0 to move window forward, 1 to send packets, 2 to stop the client

			// 			for(int i =0; i<WINSIZE; i++)
			// 			{
			// 				if(ack_received[i]==0)
			// 				{
			// 					flag = 1;
			// 					break;
			// 				}
			// 				if(ack_received[i]==2)
			// 				{
			// 					flag = 2;
			// 					break;
			// 				}
			// 			}
						
			// 			if(flag == 0)
			// 			{
			// 				//move window forward

			// 				//set all packets to not yet read
			// 				for(int i = 0; i<WINSIZE; i++)
			// 				{
			// 					ack_received[i]=2;
			// 				}

			// 				//read packets from file
			// 				for(int i = 0 ; i<WINSIZE; i++)
			// 				{
			// 					tempseq = ftell(fp);
			// 					tempsize = fread(buffer, 1, PACKET_SIZE, fp);
			// 					if(feof(fp))
			// 					{
			// 						templast = 1;
			// 					}

			// 					sprintf(data1.payload, "%s", buffer);
			// 					data1.size = tempsize;
			// 					data1.seq = tempseq;
			// 					data1.last = templast;
			// 					data1.packettype = data;
			// 					data1.channel = (data1.seq/PACKET_SIZE)%2;

			// 					window[i] = data1;
			// 					ack_received[i] = 0;

			// 					if(templast)
			// 						break;
			// 				}
			// 				//send packets of new window

			// 				for(int i =0; i<WINSIZE; i++)
			// 				{
			// 					data1 = window[i];
			// 					if(ack_received[i]==0)
			// 					{
			// 						if(data1.channel==0)
			// 						{
			// 							while((sendto(fd2, &data1, sizeof(data1), 0 , (struct sockaddr*) &relay2, sizeof(relay2)))<0);
			// 							{
			// 								printerror("sending packet from first channel\n", 0);
			// 							}
			// 							trace(data1);
			// 						}
			// 						else
			// 						{
			// 							while((sendto(fd1, &data1, sizeof(data1), 0 , (struct sockaddr*) &relay1, sizeof(relay1)))<0);
			// 							{
			// 								printerror("sending packet from first channel\n", 0);
			// 							}
			// 							trace(data1);
			// 						}
			// 					}
			// 				}
			// 			}
			// 			else if(flag==2)
			// 			{
			// 				//exit
			// 				break;
			// 			}
			// 		}
			// 	}
			// }	
		}
		else
		{
			//timeout for both the channels

			for(int i =0; i<WINSIZE; i++)
			{
				data1 = window[i];
				if(ack_received[i]==0)
				{
					if(data1.channel==0)
					{
						while((sendto(fd1, &data1, sizeof(data1), 0 , (struct sockaddr*) &relay2, sizeof(relay2)))<0);
						{
							// printerror("sending packet from first channel\n", 0);
						}
						trace(data1);
					}
					else
					{
						while((sendto(fd1, &data1, sizeof(data1), 0 , (struct sockaddr*) &relay1, sizeof(relay1)))<0);
						{
							// printerror("sending packet from second channel\n", 0);
						}
						trace(data1);
					}
				}
			}
		}
	}
	
	close(fd1);
	// close(fd2);
}
