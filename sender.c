#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>

#include "packet.h"

#define NTHREADS 3
#define TIMEOUT 2

///////////////////////////////////////////////////////////
// 					GLOBAL VARIABLES					 //
///////////////////////////////////////////////////////////

FILE* fp = NULL;				// Pointer to the file being read.
size_t fileSize;				// Size of aforementioned file.	
char* filebuf = NULL;			// Buffer for storing file.
char* messageBuf = NULL;		// Buffer for segments being read/written.

int sockfd;
int newsockfd;
int portno;
int pid;

struct sockaddr_in serv_addr;
struct sockaddr_in cli_addr;
socklen_t clilen;

int nextPacket;					// Next packet to be read.
int maxPacket;					// Max number of packets.
int lastAck = 0;				// The last acknowledged packet.

int base, end, cwnd;			// Beginning, end, and size of window (in bytes).

double pl, pc;					// Probabilities of failure.
int retransmitting;				// 1 if the sender is currently retransmitting.

time_t* timeoutTimes;			// Time allowed for threads to time out.

///////////////////////////////////////////////////////////
// 					HELPER FUNCTIONS					 //
///////////////////////////////////////////////////////////

// Output an error and exit the program.
void error(char* s)
{
	perror(s);
	exit(1);
}

// Return the minimum of two sizes.
size_t minimum (size_t a, size_t b)
{
	return (a > b) ? b : a;
}

// Determines whether or not a packet should be lost/corrupted due in
// correspondence with the supplied probabilites.
//
// Return Values:
//     0 - Fine
//     1 - Lost
//     2 - Corrupted
int isPacketBad(double pl, double pc)
{
	if (pl < 0.0 || pc < 0.0 || pl > 1.0 || pc > 1.0)
	{
		error("Invalid probabilities given");
	}

	// Get the obvious ones out of the way.
	if (pl == 1.0) { return 1; }
	if (pc == 1.0) { return 2; }
	if (pc == 0.0 && pl == 0.0) { return 0; }
	
	// Roll the dice...
	srand48((long) time(NULL));
	double rl = rand() / ((double) RAND_MAX);
	double rc = rand() / ((double) RAND_MAX);

	if (rl < pl) { return 1; }
	if (rc < pc) { return 2; }
	else { return 0; }
}

///////////////////////////////////////////////////////////
// 					SENDER/SERVER CODE					 //
///////////////////////////////////////////////////////////

int processDatagram();
void readFile(char* fileName);
void retransmitPackets(int retransmitStart);
void sendPacket(int packetNum);
void resetSender();

// Main functio for thread responsible for handling and sending segments.
void* senderMain(void* b)
{
	// Open socket.
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0)
	{
		error("ERROR opening socket");
	}
	bzero((char*) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);

	// Bind socket to port.
	if (bind(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0)
	{
		error("ERROR on binding");
	}
	clilen = sizeof(cli_addr);

	// Receive incoming segments, and take care of them.
	int done = 0;
	while (1) {
		recvfrom(sockfd, messageBuf, PACKET_SIZE, 0, (struct sockaddr *) &cli_addr, &clilen);
		done = processDatagram();
		if (done)
		{
			printf("Done\n");
			resetSender();
		}
	}
	pthread_exit(0);
}

// Function for thread responsible for watching thread timeouts.
void* senderTimer(void* b)
{
	while (1)
	{
		sleep(1);
		if (timeoutTimes != NULL &&
			timeoutTimes[base] != 0 &&
			difftime(time(NULL), timeoutTimes[base]) >= 0)
		{
			printf("Packet %d has timed out, retransmitting...", base * DATA_SIZE);
			retransmitPackets(base);
		}
	}
	pthread_exit(0);
}

// The "sender" program is called with four arguments:
//    argv[1] - The port number to listen on.
//    argv[2] - The window size (in bytes).
//    argv[3] - The probability that any given packet is lost.
//    argv[4] - The probability that any given packet is corrupted.
int main(int argc, char* argv[])
{
	if (argc < 5)
	{
		printf(stderr, "Usage: ./sender <port number> <window size> <packet loss probability> <packet corruption probability>\n");
		exit(1);
	}

	// Initialize global variables.
	messageBuf = malloc(PACKET_SIZE);
	base = 0;
	portno = atoi(argv[1]);
	cwnd = (atoi(argv[2]) / DATA_SIZE) - 1;
	end = cwnd;
	retransmitting = 0;
	pl = atof(argv[3]);
	pc = atof(argv[4]);

	// Initailize threads.
	int tid;
	pthread_t threads[NTHREADS];
	pthread_attr_t attr;
	pthread_attr_init(&attr);

	// One thread is responsible for keeping an eye out for any incoming
	// segments. Another one makes sure that segments don't take to long
	// to be acknowledged.
	for (tid = 1; tid < NTHREADS; tid++)
	{
		if (tid == 1)
		{
			pthread_create(&threads[tid], &attr, senderMain, NULL);
		}
		else if (tid == 2)
		{
			pthread_create(&threads[tid], &attr, senderTimer, NULL);
		}
	}

	// Clean up threads when done.
	for (tid = 1; tid < NTHREADS; tid++)
	{
		pthread_join(threads[tid], NULL);
	}
	pthread_attr_destroy(&attr);
	pthread_exit(NULL);
}

// Process an incoming segment (which may be a request for a file, or an
// acknowledgment.
//
// Returns 0 if nothing interesting happens, or 1 if a file is done being
// transferred.
int processDatagram()
{
	packet_t p = deserialize_packet(messageBuf);
	
	// A file is requested.
	if (p.type == TYPE_REQUEST)
	{
		char* fileName = p.data;
		printf("Received requst for file: %s\n", fileName);

		// Read from the file, and send packets while they fall into the
		// allowed window size.
		readFile(fileName);
		base = 0;
		nextPacket = 0;
		end = cwnd;
		for (nextPacket; nextPacket <= end; nextPacket++)
		{
			if (nextPacket == maxPacket)
			{
				break;
			}
			sendPacket(nextPacket);
		}
	}

	// Acknowledgement that client received packet.
	else if (p.type == TYPE_ACK)
	{
		// Potentially ignore packet due to probabilities of being lost or
		// corrupted.
		int ignore = isPacketBad(pl, pc);
		if (ignore == 1)
		{
			printf("Packet with ACK %d was lost, ignoring\n", p.packet_num);
			return 0;
		}
		else if (ignore == 2)
		{
			printf("Packt with ACK %d was corrupted, ignoring\n", p.packet_num);
			return 0;
		}
		else
		{
			printf("Received ACK %d\n", p.packet_num);
		}

		if (p.packet_num == maxPacket - 1)
		{
			return 1;
		}
	
		// Sync the global variables to reflect that a packet has been received.
		if (retransmitting != 1 &&
			p.packet_num >= base &&
			nextPacket != maxPacket)
		{
			int diff = p.packet_num - base;
			base = p.packet_num + 1;
			end += diff + 1;
			for (nextPacket; nextPacket <= end; nextPacket++)
			{
				if (nextPacket == maxPacket)
				{
					break;
				}
				sendPacket(nextPacket);
			}
		}
	}
	else
	{
		error("Invalid packet type\n");
	}

	return 0;
}

// Read data from a requested file into the file buffer.
void readFile(char* fileName)
{
	// Attempt to find and open the file.
	fp = fopen(fileName, "r");
	if (fp == NULL)
	{
		printf("File not found\n");
		return;
	}

	// Find the size of the file, and read it's contents into the buffer.
	fseek(fp, 0, SEEK_END);
	fileSize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	filebuf = (char*) malloc(fileSize);
	fread(filebuf, 1, fileSize, fp);

	// Determine the number of packets required to transmit the file.
	maxPacket = fileSize / DATA_SIZE;
	timeoutTimes = malloc(maxPacket * sizeof(time_t) + 1);
	bzero(timeoutTimes, maxPacket * sizeof(time_t) + 1);

	if (fileSize % DATA_SIZE != 0)
	{
		maxPacket++;
	}

	fclose(fp);
}

// Send a single packet to the receiver.
void sendPacket(int packetNum)
{
	// Determine how far into the data we are reading, and the length of
	// the content to be read.
	int offset = packetNum * DATA_SIZE;
	size_t contentLength = minimum(DATA_SIZE, fileSize - offset);

	// Create the packet that will be sent.
	packet_t p;
	p.type = TYPE_MESSAGE;
	p.packet_num = packetNum;
	p.packet_length = contentLength;
	p.checksum = 0;

	// If this is the end of the file, change the packet type to reflect it.
	if (offset + contentLength >= fileSize)
	{
		p.type =TYPE_END_OF_FILE;
	}

	// Serialize the packet and copy it into the buffer.
	memcpy(p.data, filebuf + offset, contentLength);
	bzero(messageBuf, PACKET_SIZE);
	messageBuf = serialize_packet(p);

	// Send the packet.
	printf("Sending packet %d\n", packetNum);
	sendto(sockfd, messageBuf, PACKET_SIZE, 0, (struct sockaddr *) &cli_addr, clilen);

	timeoutTimes[packetNum] = time(NULL) + TIMEOUT;
}

// Retransmit packets starting from "retransmitStart" due to packet loss or
// corruption.
void retransmitPackets(int retransmitStart)
{
	retransmitting = 1;
	base = retransmitStart;
	nextPacket = base;
	end = base + cwnd;
	for (nextPacket; nextPacket <= end; nextPacket++)
	{
		if (nextPacket == maxPacket)
		{
			break;
		}
		sendPacket(nextPacket);
	}

	retransmitting = 0;
}

// After a file has been sent, reset global variables accordingly.
void resetSender()
{
	bzero(messageBuf, PACKET_SIZE);
	if (timeoutTimes != NULL)
	{
		free(timeoutTimes);	
		timeoutTimes = NULL;
	}
	if (filebuf != NULL)
	{
		free(filebuf);
		filebuf = NULL;
	}
}

