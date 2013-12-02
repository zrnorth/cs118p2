
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
#include <strings.h>

#include "packet.h"
#define RECEIVER_PORT 8080 // for testing and stuff



void error(char *msg)
{
    perror(msg);
    exit(0);
}

int main(int argc, char *argv[])
{
    int sockfd; //Socket descriptor
    int portno, n;
    struct sockaddr_in si_rcvr, si_sender;
    int slen = sizeof(si_sender);

    char buffer[256];
    if (argc < 4) {
       fprintf(stderr,"usage %s sender_hostname sender_portnumber filename\n", argv[0]);
       exit(0);
    }

    char* sender_hostname = argv[1];
    int sender_portnumber = atoi(argv[2]);
    char* filename = argv[3];
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) error("ERROR opening socket.");

    memset((char*) &si_sender, 0, sizeof(si_rcvr));
    si_sender.sin_family = AF_INET;
    si_sender.sin_port = htons(sender_portnumber);

    struct hostent *server; // to get the server info
    server = gethostbyname(sender_hostname);
    if (!server) error("hostname lookup failed");

    // construct the packet
    struct PACKET request_packet;
    request_packet.source_port = RECEIVER_PORT;
    request_packet.dest_port = (unsigned int)sender_portnumber;
    request_packet.type = TYPE_REQUEST; //requesting a filename

    if (sizeof(filename) > sizeof(request_packet.data)) // can't fit
        error("filename is too long");
    strncpy(request_packet.data, filename, sizeof(filename)); //copy in the data

    request_packet.packet_num = 0; //don't care
    request_packet.packet_length = sizeof(filename) + HEADER_SIZE; //length of the packet
    request_packet.checksum = 0; //TODO don't care for now

    char* sp = serialize_packet(request_packet);
    // now, the request packet is composed. send it to the "sender"
    //TODO debug
    printf("%s\n", sp);
    //end debug

    sendto(sockfd, sp, PACKET_SIZE, 0, (struct sockaddr*) &si_sender, slen);
    close(sockfd);
    return 0;
}
