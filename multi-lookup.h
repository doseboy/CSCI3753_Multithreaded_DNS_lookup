#ifndef MULTI_LOOKUP_H
#define MULTI_LOOKUP_H


#include <pthread.h>		// pthreads!
#include <stdlib.h>		
#include <stdio.h>		// for file I/O, stderr
#include <errno.h>		// for errno
#include <unistd.h>		
#include "util.h"		// for DNS lookup
#include "queue.h"		// queue for hostnames.


void* req();

void* res();

int main(int argc, char* argv[]);

#endif
