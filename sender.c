/**
 * Zach North - 603 885 768
 * Rory Snively - 803 889 336
 *
 * CS 118 - Project 1
 * UCLA, Fall 2013
 *
 *
 * This code runs a simple server in the internet domain using TCP. The port
 * number is passd as an argument. Once a port is established, the server runs
 * until shut down. Every time a new connection is made, the server forks a new
 * process to handle it.
 */

#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "packet.h"


// Maximum file path size (bytes).
const int PATH_BUFFER_SIZE = 100;

// Prints an error message, and then exits.
void error(char* msg)
{
    perror(msg);
    exit(1);
}

int file_size; // global; holds the size of the file in bytes
char* file_buffer; //Global; holds the file to be served.
int max_num_pkts; // Global; holds the number of packets in the file.
int sockfd, newsockfd, portno, pid;
socklen_t clilen;
struct sockaddr_in serv_addr, cli_addr;

// helper function
int min(int a, int b)
{
    if (a < b) return a;
    else return b;
}

// Sends a packet on the wire to the receiver
void sendPacket(int packet_num)
{
    packet_t p;
    p.type = TYPE_MESSAGE;
    p.packet_num = packet_num;
    p.checksum = 0; // todo

    // need to get the length of the packet (usually 1000, unless last pkt.)
    size_t length = min(DATA_SIZE, file_size - (packet_num * DATA_SIZE));
    // copy in the data
    memcpy(p.data, file_buffer+(packet_num * DATA_SIZE), length);

    p.packet_length = length + HEADER_SIZE;

    // Serialize the packet to be sent
    char* sp = serialize_packet(p);

    // Send the packet
    printf("Sending packet #%i\n", packet_num);
    sendto(sockfd, sp, HEADER_SIZE + length, 0, (struct sockaddr*)&cli_addr, clilen);
}


// Subroutine of processFile that checks if a file exists.
// If it does, reads the file contents into a buffer.
// Else returns null.
void getFile(char* filename)
{
    FILE* fp = fopen(filename, "r");
    if (fp == NULL)
    {
        error("File not found.");
    }

    // Get the total length of the file
    fseek(fp, 0, SEEK_END);
    file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    // Create a buffer and copy over the bytes
    file_buffer = malloc(file_size);
    fread(file_buffer, 1, file_size, fp);

    // Need to determine number of packets to send
    max_num_pkts = file_size / DATA_SIZE;
    if (file_size % DATA_SIZE != 0)
        max_num_pkts++; //one more packet to get the remaining needed.

    fclose(fp);
}
// Checks if a file exists in the system and serves it; else returns error.
int processFile(char* rcvd_pkt)
{
    // Deserialize the packet into a struct to make it easier to read.
    packet_t p = deserialize_packet(rcvd_pkt);
    if (p.type == TYPE_REQUEST) // looking for a file.
    {
        char* filename = p.data;
        printf("Received request for file %s\n", filename);

        getFile(filename);

        // Serve the file, one packet at a time.
        // for now we just send them all once, with no going back
        int base = 0;
        int next = 0;
        for (next; next < max_num_pkts; next++)
        {
            sendPacket(next);
        }
    }
    else if (p.type == TYPE_ACK) // acking a packet that was sent
    {
        printf("Received ACK\n");
        if (p.packet_num == max_num_pkts-1) // got the last packet
            return 1;

        //TODO logic for retransmitting here.
    }
    else // wrong packet sent
    {
        printf("Error: wrong packet type received\n");
    }
    return 0;
}

// Reset starts the timers over
void reset()
{
    // TODO implement
}
// Establishes the given socket as an access point to our server. Then spawns
// off processes for each connection that is made to serve the proper response.
int main(int argc, char* argv[])
{
    // Make sure the arguments are correctly supplied.
    if (argc < 2)
    {
        fprintf(stderr, "ERROR, no port provided\n");
        exit(1);
    }

    // Connect to a socket.
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        error("ERROR opening socket");
    }

    // Establish correspondence between socket and supplied port number.
    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
    {
        error("ERROR on binding");
    }

    // Only listen on up to 5 processes at a time.
    //listen(sockfd, 5);

    int finishedTransmitting;
	char* buffer[PACKET_SIZE];
    clilen = sizeof(cli_addr);
	while(1)
	{
        recvfrom(sockfd, buffer, PACKET_SIZE, 0, (struct sockaddr*)&cli_addr, &clilen);
        finishedTransmitting = processFile(buffer);
        if (finishedTransmitting)
        {
            printf("done\n");
            reset();
            break;
        }
    }
    return 0;
}
