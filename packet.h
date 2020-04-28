#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <stdbool.h>

#define PACKET_SIZE 100		//bytes
#define PERCENT_LOSS 10		//percentage
#define TIMEOUT 0			//seconds
#define MTIMEOUT 100000			//microseconds
#define SOURCE "text.txt"
#define DEST "dest.txt"
#define WINSIZE 100			//window size at sender as well as receiver

typedef enum packettype { data = 1, ak = 2} type;

typedef struct
{
	char payload[PACKET_SIZE];
	int size;
	int seq;
	int channel;
	int dest;
	bool last;
	type packettype;
} packet;


