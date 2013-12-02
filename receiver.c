
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

char* output_file; // Global. keeps track of the name of the file we are writing to


void error(char *msg)
{
    perror(msg);
    exit(0);
}

// Sends a packet to the sender process requesting a file.
// This is the "initial" packet that starts the chain
void sendRequestPacket(char *filename,
                       int sockfd,
                       struct sockaddr_in si_sender)
{
    // Construct a request packet
    packet_t request_packet;
    if (strlen(filename) > DATA_SIZE)
        error("filename is too long");

    strcpy(request_packet.data, filename);
    request_packet.packet_length = sizeof(filename) + HEADER_SIZE;
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
                   struct sockaddr_in si_sender)
{
    // Construct the ack packet
    packet_t ack_packet;
    // the data element in the ack packet is null, because just sending ack
    ack_packet.packet_length = HEADER_SIZE;
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

// Puts the packet's information into the output file.
void putIntoFile(int length, char* raw_pkt)
{
    int i;
    for(i = HEADER_SIZE; i < length; i++)
    {
        // get 1 byte at a time and write to file
        char byte = *(raw_pkt + i);

        //TODO debug
        printf("%c\n", byte);
        //end TODO

        fprintf(output_file, "%c", byte);
        fflush(stdout); //just in case
    }
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

    // Setup the file to be written to. Call it "out-" + filename
    char* output;
    asprintf(&output, "%s%s", "out-", filename);
    output_file = fopen(output, "w"); // shouldn't fail because opening for writing
    free(output);

    // Send the initial request packet
    sendRequestPacket(filename, sockfd, si_sender);

    // Now waiting for packet back from sender

    char pkt[1000];
    socklen_t slen = sizeof(si_sender);

    //Debug: just receive one and print it.
    recvfrom(sockfd, pkt, PACKET_SIZE, 0, (struct sockaddr*) &si_sender, &slen);
    printf("Packet received: %x\n", pkt);
    packet_t p = deserialize_packet(pkt);
    printf("Packet length: %i\n", p.packet_length);
    sendAckPacket(p.packet_num, sockfd, si_sender);
    putIntoFile(p.packet_length, pkt);

    close(sockfd);
    return 0;
}
