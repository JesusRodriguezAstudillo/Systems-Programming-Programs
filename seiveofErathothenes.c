/*
Author:		Jesus Rodriguez
Program:	minor4.c
Date:		4/16/2017
Description:	This program is used to determine the number of prime numbers between 0 and value greater
		than or equal to 2. The program uses the sieve of Eratosthenes algorithm to determine the
		prime numbers. The program also uses pthreada to hold and determine the prime numbers
		using pipeline methodology to mark all prime numbers until the value of a thread's
		current prime number squared is greater than the range of values. All prime numbers are
		printed in order by using a method similar to preorder tree traversal.
*/
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

int* ARRAY_PTR; // a global array pointer
int SIZE; // a global int representing the size of an array
pthread_mutex_t MUTEX;

void* find_prime(void* current_prime);

int main(int argc, char** argv)
{
	// check if the number of arguments is correct
	if(argc != 2)
	{
		printf("Usage:\t./minor4 [N]\n");
		exit(0);
	}

	int i; // a looping variable
	int error; // a variable to check for errors
	int number = atoi(argv[1]); // the max range of prime numbers

	if(number < 2)
	{
		printf("[N] must be an integer greater than 1\n");
		exit(0);
	}

	// initialize the mutex and check for errors
	error = pthread_mutex_init(&MUTEX, NULL);
	if(error)
	{
		printf("Error initializing mutex.");
		exit(0);
	}

	int array[number+1]; // create an array to hold 0 or 1s
	SIZE = number; // initialize the global size variable

	for(i = 0; i <= number; i++) array[i] = 1; // initialize the array, assume all values are prime numbers
	array[0] = array[1] = 0; // 0 and 1 are not prime numbers
	ARRAY_PTR = array; // set the global pointer to point to the created array

	int first_prime = 2; // create a variable to pass to the pthread
	pthread_t first = 2; // create an id for the pthread
	int status; // create a variable to recieve the status from the pthread join

	// create the first thread and pass the necessary variables and check for errors
	error = pthread_create(&first, NULL, find_prime, (void*)first_prime);
	if(error)
	{
		printf("Error creating the first thread.");
		exit(0);
	}

	// join the created thread and check for errors
	printf("Prime numbers of %d: { ", number);
	error = pthread_join(first, (void**)&status);
	if(error)
	{
		printf("Error joining in main.");
		exit(0);
	}
//	printf("}\n");

	// destroy the mutex and check for errors 
	error = pthread_mutex_destroy(&MUTEX);
	if(error)
	{
		printf("Error destroying mutex.");
		exit(0);
	}

	pthread_exit(NULL);
}
/*
Name:		find_prime
Parameters:	a void* representing a prime number passed from the previous thread
Return:		
Description:	This function is used to determine and hold prime numbers using pthreads.
		The function first determines if the value of the current prime
		squared is greater than the value entered. If it is, the function
		knows that all prime numbers have been marked so it then determines
		if there are more prime numbers. If there are no more prime numbers,
		the thread exits otherwise it finds the next prime number using recursion.
		If the current prime number squared is not greater than the number entered,
		the thread must adjust the global array so it locks it while updating
		its values. This is done using pipeline methodology since a thread only
		updates the values of its prime number. Once the values are updated, the
		thread determines the next prime number and passes it, recursively, to
		another thread. The thread waits by joining before exiting. In order to
		display the prime numbers in order, the function uses a method similar to
		preorder tree traversal.
*/
void* find_prime(void* current_prime)
{
	int prime = (int)current_prime; // get the value of the current prime number
	int i, j; // looping variables
	int error;
	int status;// a varible to determine the status of the return
	int next_prime; // the next prime number
	pthread_t next_id; // the id of the next thread

	printf("%d ", prime); // print now for preorder traversal

	// determine if the all values in the array have been marked
	if(prime*prime > SIZE) // using sieve of Eratosthenes algorithm, if the current prime number squared is greater than the value of the input, all prime numbers have been marked
	{
		if(prime == SIZE)
		{
			printf(" }\n");
			pthread_exit((void*)prime); // if the current prime number is the last number in the list, exit
		}
		else // there may be prime numbers greater than the current one
		{
			int last = 1; // an int representing if the current prime number is the last prime number, assume true
			
			pthread_mutex_lock(&MUTEX); // lock the global pointer
			// loop through and find the next prime number
			for(i = prime+1; i <= SIZE; i++)
			{
				if(ARRAY_PTR[i] == 1) // if the value of the array is true
				{
					last = 0; // the current prime number is not the last one
					break; // break from the loop
				}
			}
			pthread_mutex_unlock(&MUTEX); // unlock the global pointer

			next_id = next_prime = i; // assign the next prime number to a variable and the thread id

			if(last == 1)
			{
				printf("}\n");
				pthread_exit((void*)prime); // there are no more prime numbers so exit
			}
			else pthread_create(&next_id, NULL, find_prime, (void*)next_prime); // recursively call the find prime function
		}
	}
	else
	{
		pthread_mutex_lock(&MUTEX); // lock the global pointer
		for(i = prime, j = prime*prime; j <= SIZE; ++i, j = prime*i) ARRAY_PTR[j] = 0; // mark all multiples of the current prime number as false
		pthread_mutex_unlock(&MUTEX); // unlock the global pointer

		pthread_mutex_lock(&MUTEX); // lock the global pointer
		for(i = prime+1; ARRAY_PTR[i] == 0; i++); // get the next prime number
		pthread_mutex_unlock(&MUTEX); // unlock the global pointer
		next_id = next_prime = i; // assign the prime number to parameter and the pthread

		// recursively find the next prime number
		pthread_create(&next_id, NULL, find_prime, (void*)next_prime);
	}

	// join the child thread
//	pthread_join(next_id, (void**)&status); /*** using thread join limits the number of threads that can be created so I chose to remove it ***/

	pthread_exit((void*)prime); 
}
