#include <time.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include "util.h"
#include "multi-lookup.h"
#include "queue.h"


#define MAX_INPUT_FILES 10
#define MAX_RESOLVER_THREADS  10
#define MAX_REQUESTER_THREADS 5
#define MAX_NAME_LENGTH 1025

#define ERR 0
#define USAGE "multi-lookup <# req threads> <# resolver threads> <requester logfile> <resolver logfile> [<input files>...]"

/*
Reason for static initialization of mutex/cond vars. In my case there is no need for recursive calls and special properties as iterated below:

----------------------------------------------------------------------------------------------------

By older versions of the POSIX standard the first method with an initializer is only guaranteed to work with statically allocated variables, not when the variable is an auto variable that is defined in a function body. Although I have never seen a platform where this would not be allowed, even for auto variables, and this restriction has been removed in the latest version of the POSIX standard.

The static variant is really preferable if you may, since it allows to write bootstrap code much easier. Whenever at run time you enter into code that uses such a mutex, you can be assured that the mutex is initialized. This is a precious information in multi-threading context.

The method using an init function is preferable when you need special properties for your mutex, such as being recursive e.g or being shareable between processes, not only between threads.
*/

static	int	Argc;				// Global copy of main()'s argc for requester thread use
static	char	**Argv;				// Global copy of main()'s argv for requester thread use
static	int	Next_argv_index;		// argv[Next_argv_index] is the next candidate filename
static	int	Num_requester_threads;
static	int	Num_resolver_threads;
static	char	*Requester_log_filename;
static	char	*Resolver_log_filename;
static	FILE	*Requester_log;
static	FILE	*Resolver_log;
static	void	*rvv;				// Thread return value to pthread_join()

int		No_more_requests_coming = 0;	// Flag indicates no more requests will enter queue.
queue		Q; // circular buffer/queue

pthread_cond_t empty = PTHREAD_COND_INITIALIZER; //when queue is empty
pthread_cond_t full  = PTHREAD_COND_INITIALIZER; //when queue is full


pthread_mutex_t qLock   = PTHREAD_MUTEX_INITIALIZER; //for queue
pthread_mutex_t fileLock= PTHREAD_MUTEX_INITIALIZER; //for input file
pthread_mutex_t outLock = PTHREAD_MUTEX_INITIALIZER; //for output file


int thread_finir = 0;

// char *next_input_file() -- Return next input filename, or NULL if no more
char *next_input_file()
{
	char *retVal;
	pthread_mutex_lock(&fileLock);
	if (Next_argv_index < Argc) {
		retVal = Argv[Next_argv_index];
		Next_argv_index++;
	} else {
		retVal = NULL;
	}
	pthread_mutex_unlock(&fileLock);

	return retVal;
}

//takes in input file name
void* req()
{
	char	*input_filename;
	int	num_files_processed = 0;

//?	if(ERR) {fprintf(stderr, "INIT OF REQTHREADS\n");}
	while ((input_filename = next_input_file())) {
		FILE* fp = fopen(input_filename, "r");
		if(!fp)
		{
			char error[1025];
			sprintf(error, "ERROR OPENING request FILE '%s'", input_filename);
			perror(error);
			return NULL;
		}
		char host[1025];
		while(fscanf(fp, "%1024s", host)>0)
//		while (fgets(host, 1025, fp))
		{
			char* hostpointer = malloc(sizeof(host));
			strncpy(hostpointer, host, sizeof(host));
	printf(">>> enqueuing %s\n", hostpointer);
			if(ERR){ fprintf(stderr, "ENQUEUEING\n"); }
			pthread_mutex_lock(&qLock);
			while((enqueue(&Q, hostpointer)) == -1)
			{
				pthread_cond_wait(&full, &qLock);
			} 
			pthread_mutex_unlock(&qLock);
			pthread_cond_signal(&empty);
		}
		num_files_processed++;
		fclose(fp);
	}
	fprintf(stderr, "Thread (Requester) [%x] FINISHED\n", (unsigned int)pthread_self());
	return (void *)(long int)num_files_processed;
}

void* res()
{
	while(1)
	{
		char* hp;
		pthread_mutex_lock(&qLock);
		while ((hp = dequeue(&Q)) == NULL)
		{
			if (No_more_requests_coming)
			{
				fprintf(stderr, "Thread (Resolver) [%x] FINISHED\n", (unsigned int)pthread_self());
				pthread_mutex_unlock(&qLock);
				pthread_exit(NULL);
			}
			pthread_cond_wait(&empty, &qLock);
		}
		pthread_mutex_unlock(&qLock);
		pthread_cond_signal(&full);

		char host[1025];
		sprintf(host, "%s", hp);
		free(hp);
		char ipaddress[INET6_ADDRSTRLEN];
//printf(">>> Looking up Host %s\n", host);
		if(dnslookup(host, ipaddress, sizeof(ipaddress)) != 0)
		{
			fprintf(stderr, "DNS ERR\n");
printf(">>> Host %s --> IP: %s\n", host, ipaddress);
			strncpy(ipaddress, "", sizeof(ipaddress));
		}
		pthread_mutex_lock(&outLock);
		fprintf(Resolver_log, "%s,%s\n", host, ipaddress);
		pthread_mutex_unlock(&outLock);

	}
}

int main(int argc, char* argv[])
{
	int	i;
//	if(argc < 3)
//	{
//		fprintf(stderr, "ARGS LESS THAN 3\n");
//		return EXIT_FAILURE;
//	}
#if 0
	char* inputFiles[argc-2];
	pthread_t req_threads[argc-1];
	pthread_t res_threads[10];
#endif
	//Used to print out time it took to execute program.
	struct timeval tv0, tv1;
	gettimeofday(&tv0, NULL);
	// Initialize from argv[]:
	pthread_t req_threads[MAX_REQUESTER_THREADS ];
	pthread_t res_threads[MAX_RESOLVER_THREADS  ];
	Num_requester_threads  = atoi(argv[1]);
	Num_resolver_threads   = atoi(argv[2]);
	Requester_log_filename = argv[3];
	Resolver_log_filename  = argv[4];
	Argc = argc;
	Argv = argv;
	Next_argv_index = 5;
printf(">>>> Num_requester_threads=%d, Num_resolver_threads=%d, Requester_log: %s, Resolver_log: %s\n", Num_requester_threads, Num_resolver_threads, Requester_log_filename, Resolver_log_filename);

//	FILE* output = fopen(argv[(argc-1)], "w");
	Requester_log = fopen(Requester_log_filename, "w");
	if (!Requester_log)
	{
		perror("ERROR OPENING requester OUTPUT FILE");
		return -1;
	}

	Resolver_log = fopen(Resolver_log_filename, "w");
	if (!Resolver_log)
	{
		perror("ERROR OPENING resolver OUTPUT FILE");
		return -1;
	}

#if 0
	for (i=5; i < argc; i++)
	{
		printf(">>>> Input file argv[%d] = %s\n", i, argv[i]);
		inputFiles[i-5] = argv[i];	
	}
#endif

	int rv = q_init(&Q, 50);
	printf(">>> q_init() returned %d\n", rv);

	// Start Requester threads
	for (i=0; i<Num_requester_threads; i++)
	{
		int reqthreads = pthread_create(&req_threads[i], NULL, (void*) req, NULL);

		printf(">>> Created Requester Thread %d, ID=%x\n", i, (unsigned int)req_threads[i]);
		if (reqthreads)
		{
			printf("ERROR CREATING REQTHREADS %d \n", reqthreads);
			exit(EXIT_FAILURE);
		} 
	}
sleep(1);

	// Start Resolver threads
	for(i=0; i < Num_resolver_threads; i++)
	{
		int resthreads = pthread_create(&res_threads[i], NULL, (void*) res, NULL);
		printf(">>> Created Resolver Thread %d, ID=%x\n", i, (unsigned int)res_threads[i]);
		if(resthreads)
		{
			printf("ERROR CREATING REQTHREADS %d \n",resthreads);
			exit(EXIT_FAILURE);
		}
	}

	// Complete/join Reqester threads
	for (i=0; i<Num_requester_threads; i++)
	{
		int rv = pthread_join(req_threads[i], &rvv);
		if (rv)
		{
			printf("ERROR JOINING REQTHREADS %d \n", rv);
			exit(EXIT_FAILURE);
		} 
		fprintf(stderr,        "Thread %x serviced %ld files.\n", (unsigned int)req_threads[i], (unsigned long int)rvv);
		fprintf(Requester_log, "Thread %x serviced %ld files.\n", (unsigned int)req_threads[i], (unsigned long int)rvv);
fprintf(stderr, "Joined requester thread %d\n", i);
	}
	fclose(Requester_log);

	fprintf(stderr, "All requester threads exited\n");

	No_more_requests_coming = 1;
	pthread_cond_broadcast(&empty);

	// Complete/join Resolver threads
	for(i=0; i<Num_resolver_threads; i++)
	{
		void *status;
		int resjoin = pthread_join(res_threads[i], &status);
		if(resjoin){printf("ERROR JOINING RESTHREADS %d\n", resjoin);}
fprintf(stderr, "Joined requester thread %d\n", i);
	}
	fclose(Resolver_log);
	thread_finir = 1;
	// Unwedge consumer threads

//printf("While(){}\n");
//while(1){}
fprintf(stderr, "Do Q_FREE:\n");
	q_free(&Q);
fprintf(stderr, "Destroy Mutexes:\n");
	pthread_mutex_destroy(&qLock);
	pthread_mutex_destroy(&fileLock);
	pthread_mutex_destroy(&outLock);
fprintf(stderr, "DONE Destroy Mutexes:\n");
	gettimeofday(&tv1,NULL);
	printf("----------------------------------------------------------------\n");
	printf("Elapsed time: %f seconds.\n", (tv1.tv_sec - tv0.tv_sec) + (tv1.tv_usec - tv0.tv_usec)/1e6);
	return EXIT_SUCCESS;
}
