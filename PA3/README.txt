Name: William Anderson
Phone: 720-237-6312
PA3

In this assignment I made a multi-threaded program in C that uses mutexes and condition variables
that resolves domain names to IP adresses. 

Data Structure for Buffer:
The data structure I used is in queue.c and queue.h which is a circular queue of pointers to memory.
There is an initializer function that builds this queue to a size of 50, and then a free function to
free the memory to avoid memory leaks. (Of course a enqueue and dequeue as well).

Implementation:
After completing the data structure I implemented my resolver and requester thread pools (res() and req()).
The requester threadpool had a helper function called nex_input_file() that would return the next input file
name. This was used to prevent requester threads from trying to access the same file. (This is implemented in
the truth value for the while loop). I used fscanf() to read lines from the file and host "string" to hold the
domain name (and a pointer to it) in order to enqueue it into my circular queue. This of course had a mutex
around it and a wait inside and a signal after. This follows common sense: wait if the queue is full and signal
that it is empty after. Once all this is done the requester returns the number of files serviced which is then used
in main() to print to the requester log.
The resolver thread was a bit more interesting to implement: Using a while(1) I dequeued from the circular queue and
then unlocked the mutual exclusion if there were no more requesters coming (and also exited). I waited if it was empty 
and then once this was done I signaled that the queue is full. Fortunately, the DNS function was thread safe, so
it was nice and easy to print to the results log a domain name and IP or an empty string if the function returned an
error (there was no IP). Of course, I printed DNS host error to the user. This thread will return nothing and
exit safely.
In the main() I used variables to get the number of requester threads and resolvers and allocated an array of thread
objects for them ,also, grabbing the appropriate argc and argv for the amount of requester and resolvers and filenames.
I opened the requester log and the results log, printed if there were errors and then went on to
do pthread_create for both the requester and resolver. Then I joined the requester threads and initialized a bool 
value to indicate that there were no more requester threads coming. After that I did a pthread_cond_broadcast() to
say hey if there are a bunch of resolvers waiting on a small amount of requesters, wake the requesters up to finish
the work to avoid deadlock. This makes sense when dealing with large amounts of threads. Then, I joined the resolvers,
freed the queue and destroyed the mutexes and printed the execution time with getttimeofday().
This implementation works super well and NOTE: I left all my debugging print statements to verify the implementations
of the circular buffer, thread safety, and show appropriate action of my program. 
>>>>>>>>>>>>>That is why there is a sleep(1) still in my code, comment this out for faster execution.

Anyways, thank you for reading this and I hope that this program implementation is what you were expecting.

Happy Grading,
William Gordon Anderson