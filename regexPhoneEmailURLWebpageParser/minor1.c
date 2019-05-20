/*
Author:		Jesus Rodriguez
Date:		2/15/2017
Assignment:	minor program 1
File:		minor1.c
Description: 	This program is used to sort command line arguments and the pass them to script in order to 
		extract phone numbers, emails, and urls from a file according to the command line arguments.
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char** argv)
{
	// char arrays that will be used to compare the command line arguments
	char url[] = "-url";
	char eml[] = "-email";
	char phn[] = "-phone";

	char *env[] = {NULL}; // no enviroment arguments will be passed to the script
	char name[] = "./RE.sh"; // the executable script

	char *sargv[] = {NULL, NULL, NULL, NULL}; // the script command line arguments
	sargv[0] = "RE.sh"; // set the name of the script file to the first variable
	sargv[1] = argv[2]; // set the file that will be passed to the script

	// check if the amount of arguments is correct
	if(argc != 3)
	{
		printf("The amount of arguments is incorrect.\n");
		printf("Usage: ./minor1 [-url|-email|-phone] input_file_name\n");
		exit(1);
	}

	// determine which command was entered
	if(strcmp(argv[1], url) == 0)
	{
		// the command asks for websites so pass the RE for websites
		sargv[2] = "(http|https)://[0-9a-zA-Z\\.]+\\.unt\\.edu/";
		execve(name, sargv, env); // search the file for urls containing .unt.edu/
		perror("execve");
		exit(1);
	}
	else if(strcmp(argv[1], eml) == 0)
	{
		// the command asks for emails so pass the RE for emails
		sargv[2] = "[0-9a-zA-Z_\\-\\.]+@unt.edu";
		execve(name, sargv, env); // search the file for email addresses containing @my.unt.edu
		perror("execve");
		exit(1);
	}
	else if(strcmp(argv[1], phn) == 0)
	{
		// the command asks for phone numbers so pass the RE for phones
		sargv[2] = "[0-9]{3}-[0-9]{3}-[0-9]{4}";
		execve(name, sargv, env); // search the file for phone numbers
		perror("execve");
		exit(1);
	}
	// the command that was entered is not valid
	else printf("The command %s is not a valid command.\n", argv[1]);

	return 0;
}
