
/*
 * The receiver sends the request message for a file, recieves message packets containing
 * the file, and sends ACKs upon each successfully received packet.
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>      // define structures like hostent
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "packet.h"
#define RECEIVER_PORT 8080 // for testing and stuff



void error(char *msg)
{
    perror(msg);
    exit(0);
}

// Sends a packet to the sender process requesting a file.
// This is the "initial" packet that starts the chain
void sendRequestPacket(char *filename,
                       int sockfd,
                       unsigned int sender_port,
                       struct sockaddr_in si_sender)
{
    // Construct a request packet
    packet_t request_packet;
    if (strlen(filename) > DATA_SIZE)
        error("filename is too long");

    strcpy(request_packet.data, filename);
    request_packet.packet_length = sizeof(filename) + HEADER_SIZE;
    request_packet.source_port = RECEIVER_PORT;
    request_packet.dest_port = sender_port;
    request_packet.type = TYPE_REQUEST;
    request_packet.packet_num = 0;
    request_packet.checksum = 0;

    // Serialize the request packet
    char* sp = serialize_packet(request_packet);

    // Send the packet to the sender
    sendto(sockfd, sp, PACKET_SIZE, 0, (struct sockaddr*)&si_sender, sizeof(si_sender));

    // Delete the memory because we don't need it anymore
    free(sp);
}

void sendAckPacket(int packet_to_ack,
                   int sockfd,
                   unsigned int sender_port,
                   struct sockaddr_in si_sender)
{
    // Construct the ack packet
    packet_t ack_packet;
    // the data element in the ack packet is null, because just sending ack
    ack_packet.packet_length = HEADER_SIZE;
    ack_packet.source_port = RECEIVER_PORT;
    ack_packet.dest_port = sender_port;
    ack_packet.type = TYPE_ACK;
    ack_packet.packet_num = packet_to_ack;
    ack_packet.checksum = 0; //TODO implement checksum

    // Serialize the ack packet
    char *sp = serialize_packet(ack_packet);

    // Send the ack packet
    sendto(sockfd, sp, PACKET_SIZE, 0, (struct sockaddr*)&si_sender, sizeof(si_sender));

    // Delete the packet memory
    free(sp);
}

int main(int argc, char *argv[])
{
    int sockfd; //Socket descriptor
    int portno, n;
    struct sockaddr_in si_rcvr, si_sender;


    if (argc < 4) {
       fprintf(stderr,"usage %s sender_hostname sender_portnumber filename\n", argv[0]);
       exit(0);
    }

    char* sender_hostname = argv[1];
    int sender_portnumber = atoi(argv[2]);
    char* filename = argv[3];
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) error("ERROR opening socket.");

    // Setup the connection info, so can connect to sender
    bzero((char*)&si_sender, sizeof(si_sender));
    si_sender.sin_family = AF_INET;
    inet_aton(sender_hostname, &si_sender.sin_addr);
    si_sender.sin_port = htons(sender_portnumber);

    // quick check to see if the server exists.
    struct hostent *server; // to get the server info
    server = gethostbyname(sender_hostname);
    if (!server) error("hostname lookup failed");

    // Send the initial request packet
    sendRequestPacket(filename, sockfd, sender_portnumber, si_sender);

    // Now waiting for packet back from sender

    char pkt[1000];
    socklen_t slen = sizeof(si_sender);

    //Debug: just receive one and print it.
    recvfrom(sockfd, pkt, PACKET_SIZE, 0, (struct sockaddr*) &si_sender, &slen);
    printf("%s\n", pkt);


    close(sockfd);
    return 0;
}
