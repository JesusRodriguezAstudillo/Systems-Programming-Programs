/*
Authors:	Madison Burns, Allison Goins, Jesus Rodriguez
Date:		5/9/2017
File:	 	svrMajor.c
Description:	This program acts as the server for the program cliMajor.c. The server
		should be run on cse3.cse.unt.edu and its hardcoded to run on port 5055.
		The programs first creates the server socket and loops infinitely after
		listening so that clients can continue to connect to the server. The server
		sends some messages to assign values to the client before creating a
		thread in order to handle communications in parallel from all connected.
		clients. The server should be terminated using signal interrupt, CTRL C.
*/
#include <stdio.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

#define MAX_CLIENTS 3 // the max number of clients
#define MAX_NAME_LENGTH 20 // the max length of a username

typedef struct info
{
	pthread_t thread_ID; // a client's thread_id
	int socketfd; // a client's socket descriptor
	int ID; // the client's ID to identify their socket descriptor and their username
}client_info; // a struct to hold a client's information

int COUNT = 0; // an int to keep track of connected users
int CLIENTS[MAX_CLIENTS]; // an array of socket descriptors
char USERNAMES[MAX_CLIENTS][MAX_NAME_LENGTH]; // an array of user name strings
pthread_mutex_t MUTEX; // a global mutex

void* client_handler(void* info);
void set_username(int socket_fd, int position);
void get_file(int client_skt, char* file);
void put_file(int client_skt, char* file);

int main()
{
	int server_socket; // socket descriptor
	int client_socket; // client socket descriptor
	int cli_addr_size; // client address size
	int error; // error checker
	struct sockaddr_in svr_addr; // server socket address
	struct sockaddr_in cli_addr; // client socket address
	int portnum; // port number
	int i; // looping variable
	pthread_attr_t attr; // a pthread attribute to set detached state

	// initialize variables
	portnum = 5055; // the port number 
	pthread_mutex_init(&MUTEX, NULL); // initialize the mutex
	pthread_attr_init(&attr); // initialize the attribute
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED); // set the threads as detached
	for(i = 0; i < MAX_CLIENTS; i++)
	{
		CLIENTS[i] = -1; // initialize CLIENTS array
		memset(USERNAMES[i], 0, MAX_NAME_LENGTH); // initialize USERNAMES array
	}

	server_socket = socket(AF_INET, SOCK_STREAM, 0); // make a socket
	if(server_socket < 0)
	{
		printf("socket failed\n"); // check for socket errors
		exit(0);
	}

	// allow the server to reuse the address
	int key = 1;
	setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &key, sizeof(key));

	// set up the address of the server socket
	bzero((char*)&svr_addr, sizeof(svr_addr));
	svr_addr.sin_family = AF_INET; //address family
	svr_addr.sin_addr.s_addr = INADDR_ANY; // ip
	svr_addr.sin_port = htons(portnum);

	// bind to socket and check for errors
	if(bind(server_socket, (struct sockaddr *) &svr_addr, sizeof(svr_addr)) < 0)
	{
		printf("Error binding.\n");	
		exit(0);
	}
	
	// listen for socket and check for errors
	error = listen(server_socket, 5);
	if(error < 0)
	{
		printf("Error Listening.\n");
		exit(0);
	}

	while(1)
	{
		printf("Waiting for connections...\n");

		// accept socket connection
		cli_addr_size = sizeof(cli_addr);
		client_socket = accept(server_socket, (struct sockaddr *) &cli_addr, &cli_addr_size);
		COUNT++; // increase the count
		if(client_socket < 0) // check for errors connecting
		{
			printf("Error accepting connection.\n");
			close(client_socket);
			continue; // try to get a new connection
		}
		else if(COUNT > MAX_CLIENTS) // check if the limit of clients has been reached
		{
			printf("Client limit exceeded. Connection rejected.\n");
			write(client_socket, "REJECTED", strlen("REJECTED"));
			close(client_socket);
			continue; // try to get a new connection
		}
		write(client_socket, "SUCCESS", strlen("SUCCESS")); // the user was added to the server

		for(i = 0; CLIENTS[i] != -1; i++); // find the smallest available position

		// assign values to new client
		client_info new_client; 
		new_client.socketfd = client_socket; // assign the socket descriptor
		new_client.ID = new_client.thread_ID = i+1; // assign the ID
	 	set_username(client_socket, i); // call the get username function

		CLIENTS[i] = client_socket; // place socket descriptor in array
		printf("Connected client: %s\n", USERNAMES[i]);

		// send which value to assign to LOCK on the client side
		if(COUNT == 1) write(client_socket, "0", strlen("0"));
		else write(client_socket, "1", strlen("1"));

		// create a pthread
		pthread_t tID = i+1;
		pthread_create(&new_client.thread_ID, &attr, client_handler, (void*)&new_client); // create thread handler
	}

	// close the client and server sockets
	error = close(server_socket);
	if(error < 0)
	{
		printf("Error closing.\n");
		exit(0);
	}

	error = close(client_socket);
	if(error < 0)
	{
		printf("Error closing.\n");
		exit(0);
	}

	pthread_mutex_destroy(&MUTEX);

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
	FILE* get_file; // the pointer to the file in the server
	int size; // the size of the file
	int i = 0;

	// initialize the buffers to NULL
	memset(file_buffer, 0, 2048);

	get_file = fopen(file, "r"); // attempt to read the file

	// check if the file exists in the server
	if(get_file == NULL) // the file does not exist
	{
		// send error message
		write(client_skt, "Failure", strlen("Failure"));
		
		return;
	}
	else
	{
		// determine the size of the file and reset the file pointer
		fseek(get_file, 0, SEEK_END);
		size = ftell(get_file);
		rewind(get_file);

		// check if the file should be sent or not
		if(size == 0)
		{
			write(client_skt, file_buffer, 2047);
			printf("The file was empty and was not sent to the client.\n");
		}
		else
		{
			// read from the file, place it in a buffer, and send to client
			fread(file_buffer, 1, 2047, get_file);
			write(client_skt, file_buffer, strlen(file_buffer));
			while(i <= size) // while there is more of the file to send
			{
				memset(file_buffer, 0, 2048);
				fread(file_buffer, 1, 2047, get_file);
				i = i + 2047;
				write(client_skt, file_buffer, strlen(file_buffer));
			}
			memset(file_buffer, 0, 2048);
			write(client_skt, file_buffer, 2047); // send an null buffer to stop the loop
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
	char file_buffer[2048]; // the file buffer
	int bytes_read;	// the number of bytes read
	FILE* put_file; // a pointer to the file that will be written

	// initialize the buffer to NULL
	memset(file_buffer, 0, 2048);

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
		//write(client_skt, "ready", strlen("ready"));
		// read buffer from the client
		bytes_read = read(client_skt, file_buffer, 2047);
		file_buffer[bytes_read] = 0;

		/// check if the file was empty
		if(strlen(file_buffer) == 0) 
		{
			printf("Failed to receive the file.\n");
			remove(file); // delete the created file
			return;
		}
		else
		{
			// continue reading from the client and save it to the file
			while(strlen(file_buffer)!= 0) // while there is data to recieve
			{
				fputs(file_buffer, put_file);
				memset(file_buffer, 0, 2048);
				bytes_read = read(client_skt, file_buffer, 2047);
				file_buffer[bytes_read] = 0;
			}
		}
	}
	fclose(put_file);

	return;
}
/*
Name:		set_username
Parameters:	an int representing a client's socket descriptor and an int representing the 
		position in an array
Return:
Description:	This function is used to read a username from the client and loop through the USERNAMES 
		array to check if the username is available. If the name is available the the name is
		save in USERNAMES[position]. Otherwise, the function loops until a valid username is
		entered.
*/
void set_username(int socket_fd, int position)
{
	char username[MAX_NAME_LENGTH]; // the username
	int i; // a looping variable
	int valid; // an int representing if a username is available

	while(1)
	{	
		valid = 1; // assume the user name is available
		memset(username, 0, MAX_NAME_LENGTH); // clear the username buffer
		i = read(socket_fd, username, MAX_NAME_LENGTH); // wait for the username
		username[i] = 0; 

		// check if the username is already used by another user
		for(i = 0; i < MAX_CLIENTS; i++)
		{
			if(strcmp(USERNAMES[i], username) == 0) // if the username is not available
			{
				valid = 0; // the name is not available
			}
		}
	
		// if the name is available
		if(valid == 1) 
		{
			// send a message to the user and break out of the loop
			write(socket_fd, "success", strlen("success"));
			break;
		}

		// send a message to the user and wait again
		write(socket_fd, "failure", strlen("failure"));
	}

	strcpy(USERNAMES[position], username); // save the name in the global USERNAMES array

	return;
}
/*
Name:		client_handler
Parameters:	a void* that represents the information of a client
Return:
Description:	This function is used to run parallel to the parent thread and
		read responses sent by the server.
*/
void* client_handler(void* info)
{
	client_info *client = (client_info*)info; // decode the client's info
	int client_socket = client->socketfd; // the client's socket descriptor
	int ID = client->ID; // the client's ID
	char buffer[256]; // the message buffer
	char buffer2[256]; // a second buffer
	char* substring; // a substring of buffer
	int i; // looping variable
	int end; // an int to mark the end of the buffer array and set it to null

	if(COUNT > 1) // if a new client logged in
	{
		// send a message to all connected user about the new user
		memset(buffer, 0, 256);
		snprintf(buffer, 256, "%s logged into the the chat.", USERNAMES[ID-1]);
		
		// lock mutex
		pthread_mutex_lock(&MUTEX);
		for(i = 0; i < MAX_CLIENTS; i++)// loop through CLIENTS array
		{
			if((CLIENTS[i] != -1) && (i+1 != ID))// if CLIENTS != -1 and if i+1 != ID
			{
				write(CLIENTS[i], buffer, strlen(buffer));// relay the message
				write(CLIENTS[i], "UPDATE1", strlen("UPDATE1"));
			}
			if((CLIENTS[i] != -1) && (i+1 != ID))// if CLIENTS != -1 and if i+1 != ID
			{
				memset(buffer2, 0, 256);
				snprintf(buffer2, 256, "%s is logged on.", USERNAMES[i]);
				write(client_socket, buffer2, strlen(buffer2));// relay the message
			}
		}
		// unlock mutex
		pthread_mutex_unlock(&MUTEX);
	}

	while(1)
	{
		if(COUNT == 1) // send an error message if there aren't enough users
		{
			strcpy(buffer, "There are no other users connected right now.");
			write(client_socket, buffer, strlen(buffer));
		}
		// set the buffers to null
		memset(buffer, 0, 256);
		memset(buffer2, 0, 256);

		end = read(client_socket, buffer, 255); // wait for communication from the client
		buffer[end] = 0;

		// determine the client's request
		if(strncmp(buffer, "quit", 4) == 0)
		{
			// lock mutex
			pthread_mutex_lock(&MUTEX);
			
			// write a message to all users about the user exiting
			snprintf(buffer, 256, "%s logged out of the chat.", USERNAMES[ID-1]);
			printf("%s\n", buffer);

			for(i = 0; i < MAX_CLIENTS; i++)// loop through CLIENTS array
			{
				if((CLIENTS[i] != -1) && (i+1 != ID))// if CLIENTS != -1 and if i+1 != ID
				{
					write(CLIENTS[i], buffer, strlen(buffer));// relay the message
				}
			}

			memset(USERNAMES[ID-1], 0, MAX_NAME_LENGTH); // remove the username from the USERNAME array
			CLIENTS[ID-1] = -1; // mark entry in CLIENTS[ID-1] as -1

			COUNT--;// decrease COUNT
			if(COUNT == 1) // if the COUNT is reduced to 1
			{
				// update the value of LOCK for the other users
				for(i = 0; i < MAX_CLIENTS; i++)// loop through CLIENTS array
				{
					if((CLIENTS[i] != -1) && (i+1 != ID))// if CLIENTS != -1 and if i+1 != ID
					{
						write(CLIENTS[i], "UPDATE0", strlen("UPDATE0"));// relay the message
					}
				}
			}

			strcpy(buffer, "Logout...");
			write(client_socket, buffer, strlen(buffer));// relay command to the client's recieve thread
			
			// unlock mutex
			pthread_mutex_unlock(&MUTEX);
			break;
		}

		if(COUNT == 1); // ignore requests if there is only one user
		else
		{ 
			if(strncmp(buffer, "message", 7) == 0)
			{
				// extract message
				substring = buffer + 8;
			
				// lock mutex
				pthread_mutex_lock(&MUTEX);
				
				for(i = 0; i < MAX_CLIENTS; i++)// loop through CLIENTS array
				{
					if((CLIENTS[i] != -1) && (i+1 != ID))// if CLIENTS != -1 and if i+1 != ID
					{
						write(CLIENTS[i], substring, strlen(substring));// relay the message
					}
				}
				// unlock mutex
				pthread_mutex_unlock(&MUTEX);
			}
			else if(strncmp(buffer, "put", 3) == 0)
			{
				// parse the file name from command
				substring = buffer + 4;
				printf("Receiving file: %s from: %s\n", substring, USERNAMES[ID-1]);			
				// call put file function to save it on the server
				put_file(client_socket, substring);
	
				// lock mutex
				pthread_mutex_lock(&MUTEX);
	
				// send message to users to prepare them to recieve a file
				for(i = 0; i < MAX_CLIENTS; i++)
				{
					if((CLIENTS[i] != -1) && (i+1 != ID))// if CLIENTS != -1 and if i+1 != ID
					{	
						write(CLIENTS[i], buffer, strlen(buffer));
					}
				}
			
				printf("Sending file to all users...\n");
				for(i = 0; i < MAX_CLIENTS; i++)// loop through CLIENTS again
				{
					if((CLIENTS[i] != -1) && (i+1 != ID))// if CLIENTS != -1 and if i+1 != ID
					{
						get_file(CLIENTS[i], substring);// call the get_file function 
					}
				}
	
				// send a message to all user about the file sent
				snprintf(buffer2, 256, "%s sent file: %s", USERNAMES[ID-1], substring);
				for(i = 0; i < MAX_CLIENTS; i++)
				{
					if((CLIENTS[i] != -1) && (i+1 != ID))// if CLIENTS != -1 and if i+1 != ID
					{	
						write(CLIENTS[i], buffer2, strlen(buffer2));
					}
				}
				// unlock mutex
				pthread_mutex_unlock(&MUTEX);
			} 

		}
	}

	pthread_exit(NULL);
}
