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


// Maximum file name size (bytes).
const int FILENAME_SIZE = 1024;

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
	packet_t pack = deserialize_packet(request);

	int fn_size = pack.packet_length;

	// Copy the filename into a string and return it.
	char* fn = malloc(fn_size * sizeof(char));
	strncpy(fn, pack.data, fn_size);
	return fn;
}

// Response message when file isn't found.
char* not_found_response(char* request)
{
	packet_t p_req = deserialize_packet(request);
	packet_t p_res;

	p_res.source_port = p_req.dest_port;
	p_res.dest_port = p_req.source_port;
	p_res.type = 1;
	p_res.packet_num = 0;

	char* msg = "File not found.";
	p_res.packet_length = sizeof(&msg);

	strcpy(msg, p_res.data);

	return serialize_packet(p_res);
}

// Response message when file is found.
char* found_response(char* request)
{
	packet_t p_req = deserialize_packet(request);
	packet_t p_res;

	p_res.source_port = p_req.dest_port;
	p_res.dest_port = p_req.source_port;
	p_res.type = 1;
	p_res.packet_num = 0;

	char* msg = "We has that file.";
	p_res.packet_length = sizeof(&msg);

	strcpy(msg, p_res.data);

	return serialize_packet(p_res);
}

// Responds to an request segment by saying whether or not we have the file.
void handle_request(int sock)
{
    // Read the request header.
    int n;
    char request_buffer[PACKET_SIZE];
    bzero(request_buffer, PACKET_SIZE);
    n = read(sock, request_buffer, PACKET_SIZE - 1);
    if (n < 0)
    {
        error("ERROR reading from socket");
    }

    // Print the request message to the console.
    printf("Request Segment:\n%s\n", request_buffer);

    /** Generate the response message **/
    
    char* response_msg;

    // Get the desired file.
    char* filename = get_filename(request_buffer);
    char filepath[FILENAME_SIZE];
    getcwd(filepath, FILENAME_SIZE);
    strcat(filepath, filename);
    FILE *f = fopen(filepath, "r");
    
    // File not found. Return 404 message.
    if (f == NULL)
    {
        response_msg = not_found_response(request_buffer);
        n = write(sock, response_msg, strlen(response_msg));
        if (n < 0)
        {
            error("ERROR writing to socket");
        }
    }
    // Generate response message containing file contents.
    else
    {
		response_msg = found_response(request_buffer);
		n = write(sock, response_msg, strlen(response_msg));
		if (n < 0) { error("ERROR writing to socket"); }

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

// Establishes the given socket as an access point to our server. Then spawns
// off processes for each connection that is made to serve the proper response.
int main(int argc, char* argv[])
{
    // Initialize connection to port using the given socket.
    int sockfd, newsockfd, portno, pid;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;
    struct sigaction sig_action;

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
    listen(sockfd, 5);

    clilen = sizeof(cli_addr);

    // Reap all of the zombie processes that have finished their request and
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
