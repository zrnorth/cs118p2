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

// Handler for connection processes.
void connection_handler(int signal)
{
    // Wait for connection to terminate.
    while (waitpid(-1, NULL, WNOHANG) > 0)
    {
        ;
    }
}

// Retrieves the name of the file desired by a request segment.
char* get_filename(char* request)
{
	packet_t p = deserialize_packet(request);

	return p.data;
}

// Response message when file isn't found.
char* not_found_response(char* request)
{
	packet_t p_req = deserialize_packet(request);
	packet_t p_res;

	p_res.type = TYPE_MESSAGE;
	p_res.packet_num = 0;

	char* msg = "We has that file.";
	p_res.packet_length = strlen(msg) + HEADER_SIZE;
	strcpy(p_res.data, msg);
	return serialize_packet(p_res);
}

// Response message when file is found.
char* found_response(char* request)
{
	packet_t p_req = deserialize_packet(request);
	packet_t p_res;

	p_res.type = TYPE_MESSAGE;
	p_res.packet_num = 0;

	char* msg = "We has that file.";
	p_res.packet_length = strlen(msg) + HEADER_SIZE;
	strcpy(p_res.data, msg);
	return serialize_packet(p_res);
}

// Responds to an request segment by saying whether or not we have the file.
char* handle_request(char* request)
{

    /** Generate the response message **/

    char* response_msg = malloc(PACKET_SIZE);

    // Get the desired file.
    char* filename = get_filename(request);
/*    char filepath_buffer[PATH_BUFFER_SIZE];
    getcwd(filepath_buffer, PATH_BUFFER_SIZE);

printf("1\n");
	char* full_path = malloc(strlen(filepath_buffer) + strlen(filename) + 1);
printf("2\n");
	strcpy(full_path, filepath_buffer);
printf("3\n");
	strcat(full_path, filename);
*/
    FILE *f = fopen(filename, "r");
    // File not found. Return 404 message.
    if (f == NULL)
    {
        response_msg = not_found_response(request);
		return response_msg;
    }
    // Generate response message containing file contents.
    else
    {
		response_msg = found_response(request);
		return response_msg;

/*
        // Get the size of the file contents.
        fseek(f, 0, SEEK_END);          // Find end of file.
        size_t file_size = ftell(f);    // Get size.
        fseek(f, 0, SEEK_SET);          // Return to beginning of file.

        // Read contents of file.
        char* file_contents = malloc(file_size * sizeof(char));
        size_t amount_read = fread(file_contents, 1, file_size, f);

        // Determine the type of data being delivered.
        char* file_extension = get_file_extension(filename);
        char* content_type = get_content_type(file_extension);

        // Generate the response header.
        asprintf(&response_msg, "HTTP/1.1 200 OK\r\n%sContent-Length: %zi\r\n\r\n", content_type, file_size + 2);
        // Write the status and header.
        n = write(sock, response_msg, strlen(response_msg));
        if (n < 0)
        {
            error("ERROR writing response header");
        }
        // Write the contents.
        n = write(sock, file_contents, amount_read);
        if (n < 0)
        {
            error("ERROR writing file contents");
        }
*/
    }
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

        // Serve the file, one packet at a time. Return 1 when done
        // TODO for now just send one packet
        sendPacket(0);
    }
    else if (p.type == TYPE_ACK) // acking a packet that was sent
    {
        // TODO do nothing for now
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
        }
    }
}
/*
		packet_t p = deserialize_packet(buffer);
		printf("Source Port: %d\n", p.source_port);
		printf("Dest Port: %d\n", p.dest_port);
		printf("Type: %d\n", p.type);
		printf("Packet Number: %d\n", p.packet_num);
		printf("Packet Length: %d\n", p.packet_length);
		printf("Checksum: %d\n", p.checksum);
		printf("Data: %s\n", p.data);


		char* response = handle_request(buffer);
		if (sendto(sockfd, response, PACKET_SIZE, 0, (struct sockaddr*) &cli_addr, clilen) < 0)
		{
			error("ERROR on response sending");
		}
		else
		{
			printf("Sent Response\n");
		}
	}
    //clilen = sizeof(cli_addr);
*/
/*    // Reap all of the zombie processes that have finished their request and
    // received their response.
    sig_action.sa_handler = connection_handler;
    sigemptyset(&sig_action.sa_mask);
    sig_action.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sig_action, NULL) == -1)
    {
        error("ERROR on signal action");
    }

    // Receive incoming request segments, and fork new processes for them.
    while (1)
    {
        newsockfd = accept(sockfd, (struct sockaddr*) &cli_addr, &clilen);
        if (newsockfd < 0)
        {
            error("ERROR on accept");
        }

        pid = fork();
        if (pid < 0)
        {
            error("ERROR on fork");
        }

        // The child process generates the response segment for eaach request.
        if (pid == 0)
        {
            close(sockfd);
            handle_request(newsockfd);
            exit(0);
        }
        // The parent does not need the new socket, so closes it.
        else
        {
            close(newsockfd);
        }
    }

    return 0;
}
*/