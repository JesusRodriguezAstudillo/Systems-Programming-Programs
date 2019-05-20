/*
Author:		Jesus Rodriguez
Date:		4/27/2017
Program Name:	minor5_server.c
Description:	This program serves as the server for a pair of sockets. The server
		accepts or sends files depending on a request from the client. The 
		server uses port 5055 and it's expected to run on cse03.cse.unt.edu
*/
#include <stdio.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>

void get_file(int client_skt, char* file);
void put_file(int client_skt, char* file);

int main()
{
	int socket_fd; // socket descriptor
	int cli_socket; // client socket descriptor
	int cli_addr_size; // client address size
	int error; // error checker
	struct sockaddr_in svr_sock; // server socket address
	struct sockaddr_in cli_sock; // client socket address
	int portnum; // port number
	char buf[256]; // buffer to read the command and file from client
	int bytes = 0; // the number of bytes read from client
	int i; // looping variable
	char file_name[255]; // the name of the file
	char *cpy; // a pointer to iterate over buf

	portnum = 5055; // the port number
 
	memset(file_name, 0, 255); // set all characters in file_name as NULL
	memset(buf, 0, 256); // set all locations in buf as NULL

	socket_fd = socket(AF_INET, SOCK_STREAM, 0); // make a socket
	if(socket < 0)
	{
		printf("socket failed\n"); // check for socket errors
		exit(0);
	}

	// set up the address of the server socket
	bzero((char*)&svr_sock, sizeof(svr_sock));
	svr_sock.sin_family = AF_INET; //address family
	svr_sock.sin_addr.s_addr = INADDR_ANY; // ip
	svr_sock.sin_port = htons(portnum);

	// bind to socket and check for errors
	if(bind(socket_fd, (struct sockaddr *) &svr_sock, sizeof(svr_sock)) < 0)
	{
		printf("error binding\n");	
		exit(0);
	}
	
	// listen for socket and check for errors
	error = listen(socket_fd, 5);
	if(error < 0)
	{
		printf("Error Listening.\n");
		exit(0);
	}

	printf("Waiting for connection...\n");
	// accept socket connection
	cli_addr_size = sizeof(cli_sock);
	cli_socket = accept(socket_fd, (struct sockaddr *) &cli_sock, &cli_addr_size);
	if(cli_socket < 0)
	{
		printf("error accepting\n");
		exit(0);
	}

	// loop and get commands from socket
	while(1)
	{
		memset(file_name, 0, 255); // clear all entries in file_name

		printf("Waiting for command...\n");
		bytes = read(cli_socket, buf, 255); // read from client
		buf[bytes] = 0; // set the end of buf to null
		printf("Received command: %s\n", buf);

		// check the client's request
		if(strncmp(buf, "quit", 4) == 0) break;
		else if(strncmp(buf, "get", 3) == 0)
		{
			// parse the file name
			cpy = buf; // use cpy* to iterate
			for(i = 4; cpy[i] != 0; i++) file_name[i-4] = cpy[i];
			file_name[i] = 0;

			// get the file using the client socket and file name
			get_file(cli_socket, file_name);
		}
		else if(strncmp(buf, "put", 3) == 0)
		{
			// parse the file name
			cpy = buf; // use cpy* to iterate
			for(i = 4; cpy[i] != 0; i++) file_name[i-4] = cpy[i];
			file_name[i] = 0;

			// put the file using the client socket and file name
			put_file(cli_socket, file_name);
		}
		else printf("Command unrecongnized.\n");
	}

	// close the client and server sockets
	error = close(socket_fd);
	if(error < 0)
	{
		printf("Error closing.\n");
		exit(0);
	}

	error = close(cli_socket);
	if(error < 0)
	{
		printf("Error closing.\n");
		exit(0);
	}

	return 0;
}
/*
Name:		get_file
Parameters:	an int representing a client socket and a char* representing a file name as a string
Return:		
Description:	This function is used to used to read a file from the socket acting as a server.
		If the file is found the, the file is placed in a buffer and written to the client.
		Otherwise, an error message is sent to the client.
*/
void get_file(int client_skt, char* file)
{
	char file_buffer[2048]; // the buffer for the file
	char buffer[256]; // a buffer to sent a success or failure message
	FILE* get_file; // the pointer to the file in the server
	int size; // the size of the file

	// initialize the buffers to NULL
	memset(file_buffer, 0, 2048);
	memset(buffer, 0, 256);

	get_file = fopen(file, "r"); // attempt to read the file

	// check if the file exists in the server
	if(get_file == NULL) // the file does not exist
	{
		// send error message
		strcpy(buffer, "Failure");
		write(client_skt, buffer, 255);
		
		return;
	}
	else
	{
		// send success message
		strcpy(buffer, "Success");
		write(client_skt, buffer, 255);

		// determine the size of the file and reset the file pointer
		fseek(get_file, 0, SEEK_END);
		size = ftell(get_file);
		rewind(get_file);

		// check if the file should be sent or not
		if(size == 0 || size > 2047)
		{
			write(client_skt, file_buffer, 2047);
			printf("There was an error with the file and was not sent to the client.\n");
			printf("The file was empty or was larger than 2047 bytes.\n");
		}
		else
		{
			// read from the file, place it in a buffer, and send to client
			fread(file_buffer, 1, size, get_file);
			printf("File buffer: \n%s", file_buffer);
			write(client_skt, file_buffer, 2047);
		}
	}

	fclose(get_file);
	
	return;
}
/*
Name:		put_file
Parameters:	an int representing the client socket and a char* representing a string
Return:
Description:	This function is used to read a file from a client and save it in the
		server.
*/
void put_file(int client_skt, char* file)
{
	char buffer[2048]; // the file buffer
	int bytes_read;	// the number of bytes read
	FILE* put_file; // a pointer to the file that will be written

	// initialize the buffer to NULL
	memset(buffer, 0, 2048);

	// attempt to open file
	put_file = fopen(file, "w");

	// check for errors opening the file
	if(put_file == NULL)
	{
		printf("Failed to open the file.\n");
		return;
	}
	else
	{
		// read buffer from the client
		bytes_read = read(client_skt, buffer, 2047);
		buffer[bytes_read] = 0;

		/// check if the file was empty
		if(strlen(buffer) == 0) 
		{
			printf("Failed to recieve the file.\n");
			remove(file); // delete the created file
			return;
		}
		else
		{
			printf("Received file:\n%s\n", buffer); // print the buffer on the server side
			fputs(buffer, put_file); // write the file in the server
		}
	}
	fclose(put_file);

	return;
}
