/*
Authors:	Madison Burns, Allison Goins, Jesus Rodriguez
Date:		5/9/2017
Program Name:	cliMajor.c
Description:	This program serves as the client file to connect to svrMajor.c. The client
		should connet to a server on cse03.cse.unt.edu on port 5055. After the
		client connects to the server, some values are assigned based on the 
		responses from the server. After that, the program creates a pthread so that
		the client can read messages from the server while the main program makes
		takes user input and relays it to the server.
*/
#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h> 
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define SERVER "129.120.151.96" // the servers IP address cse03.cse.unt.edu
#define MAX_NAME_LENGTH 16 // the max string length of the username

int LOCK = 0; // a global variable used to lock certain utilities if the cliet is the only one connected to the server

typedef struct info
{
	int socketfd; // the client's socket descriptor
	int ID;
	char username[MAX_NAME_LENGTH]; // the client's username
}client_info;

void set_username(int socket_fd, client_info* client);
void* receive_thread(void* info);
void get_file(int cli_skt, char* file);
void put_file(int cli_skt, char* file, char* svr_buf);

int main()
{
	int error; // error checking varible
	int cli_sock; // client socket descriptor
	struct sockaddr_in cli_addr; // client socket address
	char buffer[256]; // the command buffer
	int portnum = 5055; // the port number
	char parse_buffer[253]; // the buffer to hold the parsed buffer
	int i,j; // looping variables
	char *cpy; // a pointer to iterate through buffer[]
	client_info new_client; // a struct to hold the clients info
	pthread_t thread_ID; // the ID for the thread
	int status; // an int used to get the return status from the pthread

	// initialize variables
	memset(buffer, 0, 256); 
	memset(parse_buffer, 0, 253);

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
		printf("Error connecting socket\n");
		exit(0);
	}

	// wait for the server to determine if it can handle more users
	read(cli_sock, buffer, 255);
	if(strcmp(buffer, "REJECTED") == 0)
	{
		printf("Connection rejected. The server cannot handle more users.\n");
		exit(0);
	}
	memset(buffer, 0, 256); 

	set_username(cli_sock, &new_client); // call the function to set the username

	read(cli_sock, buffer, 255); // wait for server to give an ID to user
	LOCK = atoi(buffer); // assign the value to the LOCK
	new_client.socketfd = cli_sock; // assign the socket to the client info struct

	pthread_create(&thread_ID, NULL, receive_thread, (void*)&new_client); // create the receiving thread

	while(1)
	{
		// clear the buffer and file name by setting them to null
		memset(buffer, 0, 256);
		memset(parse_buffer, 0, 253);

		// display menu
		printf("Usage:\n");
		printf("\tmessage [text contents] (up to 200 characters) \n");
		printf("\tput [file] (up to 200 characters) \n");
		printf("\tquit\n");
		fgets(buffer, 255, stdin); // read in the command

		// remove the newline character from buffer
		for(i = 0; buffer[i] != 10; i++);  
		buffer[i] = 0;

		// check the command
		if(strncmp(buffer, "quit", 4) == 0)
		{
			write(cli_sock, buffer, strlen(buffer));
			error = pthread_join(thread_ID, (void**)&status);
			if(error < 0)
			{
				printf("Error joining the recieve_thread().\n");
				exit(0);
			}
			break;
		}
		else if(strncmp(buffer, "message ", 8) == 0)
		{
			// parse the message from the command
			cpy = buffer;
			snprintf(parse_buffer, 256, "message %s: ", new_client.username);
			
			for(i = strlen(parse_buffer), j = 8; cpy[j] != 0; i++, j++) parse_buffer[i] = cpy[j];
			parse_buffer[i] = 0;

			write(cli_sock, parse_buffer, strlen(parse_buffer)); // send the message to the server so that it can be relayed
		}
    		else if(strncmp(buffer, "put ", 4) == 0)
		{
			// parse the file name
			cpy = buffer;
			for(i = 4; cpy[i] != 0; i++) parse_buffer[i-4] = cpy[i];
			parse_buffer[i] = 0;

			// get the client ready to write to the server
			put_file(cli_sock, parse_buffer, buffer);
		}
        	else printf("Command unrecongnized.\n");
	}

	printf("Logout successful.\n");

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
	char file_buffer[2048]; // the file buffer
	int bytes_read;	// the number of bytes received
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
			printf("Failed to receive the file.\n");
			remove(file);
			return;
		}
		else // there were no errors from the file
		{
			while(strlen(file_buffer) != 0) // while there is data to recieve
			{
				fputs(file_buffer, get_file);
				memset(file_buffer, 0, 2048);
				bytes_read = read(cli_skt, file_buffer, 2047);
				file_buffer[bytes_read] = 0;
			}
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
	int i = 0; // a variable to break out of a loop
	int end;

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

		if(LOCK == 0); // if the LOCK is 0, i.e. this user is the only one connected to the server, ignore sending the file
		else // else send the file
		{
			// determine the size of the file and reset the FILE pointer
			fseek(put_file, 0, SEEK_END);
			size = ftell(put_file);
			rewind(put_file);	
	
			// check if the file should be sent
			if(size == 0)
			{
				write(cli_skt, file_buf, 2047); // send empty file as an error
				printf("The file was empty and it was not sent to the server.\n");
			}
			else
			{
				printf("Sending file to server...\n");
				sleep(1); // use sleep to allow the server to process the request

				// read the file, place it in a buffer, and write it to the server
				fread(file_buf, 1, 2047, put_file);
				write(cli_skt, file_buf, strlen(file_buf));
				while(i <= size) // while there is more data to send
				{
					memset(file_buf, 0, 2048);
					fread(file_buf, 1, 2047, put_file);
					i = i + 2047;
					write(cli_skt, file_buf, strlen(file_buf));
				}
				// send an empty buffer to stop the loop on the server side
				memset(file_buf, 0, 2048);
				write(cli_skt, file_buf, 2047);
				printf("Finished sending file.\n");
			}
		}		
	}

	fclose(put_file);

	return;
}
/*
Name:		set_username
Parameters:	an int representing the client's socket descriptor and a struct representing
		the client's information
Return:		
Description:	This function is used to prompt the user for a username which is then send
		to the server to check if it's available. If the name is availble, the loop
		breaks. Otherwise, the user is prompt again until a valid name is entered.
*/
void set_username(int socket_fd, client_info* client)
{
	char username[MAX_NAME_LENGTH]; // an array to hold the username 
	int i; // a looping variable

	while(1)
	{
		// promt for and read in the username
		memset(username, 0, MAX_NAME_LENGTH);
		printf("Enter a username, up to 18 characters: ");
		fgets(username, 19, stdin);

		// remove the new line char
		for(i = 0; username[i] != 10; i++);  
		username[i] = 0;

		// check to see if the username is available
		strcpy(client->username, username);
		printf("Checking if username %s is available...\n", username);
		write(socket_fd, username, strlen(username));

		// wait for response from the server
		i = read(socket_fd, username, 15);
		username[i] = 0;

		if(strcmp(username, "success") == 0) break; // if the username is available break
		printf("The username is already taken.\n");
	}

	printf("The username is available.\n");
	return;
}
/*
Name:		recieve_thread
Parameters:	a void* representing the user information
Return:		
Description:	This function is used to read from the server while the main program 
		runs in parallel. The thread reads from the server and branches to
		different options depending on what it reads.
*/
void* receive_thread(void* info)
{
	client_info *client = (client_info*)info; // retrive the users info
	int socketfd = client->socketfd; // the client's socket descriptor
	char buffer[256]; // the message buffer
	char* substring; // a buffer substring

	while(1)
	{
		memset(buffer, 0, 256);
		// read a request
		read(socketfd, buffer, 255);
	
		// compare buffer and determine response
		if(strncmp(buffer,"put ", 4) == 0) // if a user sent a file
		{
			// parse the file name
			substring = buffer + 4;
			printf("Receiving File: %s\n", substring);
			
			get_file(socketfd, substring);// call the get file function
		}
		else if(strncmp(buffer, "UPDATE", 6) == 0) // else if there was a change in the amount of users
		{
			// change the value of LOCK
			substring = buffer + 6;
			LOCK = atoi(substring);
		}
		else if(strcmp(buffer,"Logout...") == 0) break; // else if the buffer says it's safe to logout
		else printf("%s\n", buffer);// it must be a messge so print the buffer
	}

	pthread_exit(NULL);
}
