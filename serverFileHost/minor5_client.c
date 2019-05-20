/*
Author:		Jesus Rodriguez
Date:		4/27/2017
Program Name:	minor5_client.c
Description:	This program serves as the client side of a pair of sockets. The client
		selects whether to send a file to a server or recieve one from it. The
		client has been hard coded to connect to a server in cse03.cse.unt.edu on
		port 5055.
*/
#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>

#define SERVER "129.120.151.96" // the servers IP address cse03.cse.unt.edu

void get_file(int cli_skt, char* file);
void put_file(int cli_skt, char* file, char* svr_buf);

int main()
{
	int error; // error checking varible
	int cli_sock; // client socket descriptor
	struct sockaddr_in cli_addr; // client socket address
	char buffer[256]; // the command buffer
	int portnum = 5055; // the port number
	char file_name[253]; // the file name to send or receive
	int i; // looping variable
	char *cpy; // a pointer to iterate through buffer[]

	// create the client socket
	cli_sock = socket(AF_INET, SOCK_STREAM, 0);
	if(cli_sock < 0)
	{
		printf("Error creating client socket.\n");
		exit(0);
	}

	// initialize the address parameters
	bzero((char*) &cli_addr, sizeof(cli_addr));
	cli_addr.sin_family = AF_INET;
	cli_addr.sin_addr.s_addr = inet_addr(SERVER);
	cli_addr.sin_port = htons(portnum);

	// connect and check for errors
	error = connect(cli_sock, (struct sockaddr*) &cli_addr, sizeof(cli_addr));
	if(error < 0)
	{
		printf("Error connecting socket");
		exit(0);
	}

	while(1)
	{
		// clear the buffer and file name by setting them to null
		memset(buffer, 0, 256);
		memset(file_name, 0, 253);

		// display menu
		printf("Usage:\n");
		printf("\tget [filename]\n");
		printf("\tput [filename]\n");
		printf("\tquit\n");
		printf("Enter command: ");
		fgets(buffer, 255, stdin); // read in the command

		// remove the newline character from buffer
		for(i = 0; buffer[i] != 10; i++);  
		buffer[i] = 0;

		// check the command
		if(strncmp(buffer, "quit", 4) == 0)
		{
			write(cli_sock, buffer, strlen(buffer));
			break;
		}
		else if(strncmp(buffer, "get ", 4) == 0)
		{
			// parse the file from the command
			cpy = buffer;
			for(i = 4; cpy[i] != 0; i++) file_name[i-4] = cpy[i];
			file_name[i] = 0;

			write(cli_sock, buffer, strlen(buffer)); // send the command and the file name to the sever
	
			read(cli_sock, buffer, 255); // wait for server to determine if the file exists
			if(strncmp(buffer, "Failure", 7) == 0)
			{
				printf("Error. The server does not have the file: %s\n", file_name);
			}
			else get_file(cli_sock, file_name); // the file is in the server so prepare client to save file
		}
                else if(strncmp(buffer, "put ", 4) == 0)
		{
			// parse the file name
			cpy = buffer;
			for(i = 4; cpy[i] != 0; i++) file_name[i-4] = cpy[i];
			file_name[i] = 0;

			// get the client ready to write to the server
			put_file(cli_sock, file_name, buffer);
		}
                else printf("Command unrecongnized.\n");
	}

	// close the socket and check for errors
	error = close(cli_sock);
	if(error < 0)
	{
		printf("Error closing socket.\n");
		exit(0);
	}

	return 0;
}
/*
Name:		get_file
Parameters:	an int representing a client socket and a char* representing the name of file
Return:		
Description:	This function is used read a file buffer from a server and save
		the file in the client's side
*/
void get_file(int cli_skt, char* file)
{
	char file_buffer[2048]; // a file buffer
	int bytes_read;	// the number of bytes recieved
	FILE* get_file;	// the requested file from the server

	memset(file_buffer, 0, 2048); // initialize file_buffer to null
	get_file = fopen(file, "w"); // attempt to open the file

	// check if the file opened correctly
	if(get_file == NULL)
	{
		printf("Error receiving file.\n");
		return;
	}
	else
	{
		// read from the server and save the buffer in the file
		bytes_read = read(cli_skt, file_buffer, 2047);
		file_buffer[bytes_read] = 0;
		
		// check for empty file
		if(strlen(file_buffer) == 0)
		{
			printf("Failed to recieve the file.\n");
			remove(file);
			return;
		}
		else // there were no errors from the file
		{
			printf("File received:\n%s\n", file_buffer);
			fputs(file_buffer, get_file);
		}
	}	
	fclose(get_file);

	return;
}
/*
Name:		put_file
Parameters:	an int representing the client socket, a char* representing a file
		name as a string, and char* representing the command entered by the user
Return:
Description:	This function is used to attempt to read a file that the client has. If 
		the file is not found, an error is displayed to the user. Otherwise, the
		file is written to the server.
*/
void put_file(int cli_skt, char* file, char* svr_buf)
{
	char file_buf[2048]; // a buffer to hold a file
	FILE* put_file; // the file that will be put in the server
	int size; // the size of a file

	// initialize the file buffer to NULL
	memset(file_buf, 0, 2048);

	put_file = fopen(file, "r"); // attempt to open the file

	// check if the file exists or not
	if(put_file == NULL)
	{
		printf("Failed to open the file. The file does not exists.\n");
		return;
	}
	else
	{
		write(cli_skt, svr_buf, strlen(svr_buf)); // write the command to the server
	
		// determine the size of the file and reset the FILE pointer
		fseek(put_file, 0, SEEK_END);
		size = ftell(put_file);
		rewind(put_file);	

		// check if the file should be sent
		if(size == 0 || size > 2047)
		{
			write(cli_skt, file_buf, 2047); // send empty file as an error
			printf("There was an error with the file and it was not sent to the server.\n");
			printf("The file was empty or was larger than 2048 bytes.\n");
		}
		else
		{
			// read the file, place it in a buffer, and write it to the server
			fread(file_buf, 1, size, put_file);
			printf("Client file:\n%s\n", file_buf);
			write(cli_skt, file_buf, strlen(file_buf));
		}
	}
	fclose(put_file);

	return;
}
