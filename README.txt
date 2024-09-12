Milestone1:
Rudy Zhang: sender and general debugging
Bruce Zhang: Connection and receiver, readme

Milestone2:
Rudy Zhang: room, server member functions and general debugging
Bruce Zhang: server client thread functions, message_queues 


Approach:
Thread synchronization is important when there are multiple receivers and senders enter or leave the room, send messages to each other, and 
some users may attempt to find or create a room simultaneously.

In our server, there are several critical sections that require synchronization to avoid data inconsistencies. 
We make thread synchornization in room.cpp, message_queue.cpp, and find_or_create_room function.

The key of synchronization the data structure message_queue. This shared data structure allow the server to store the messages 
sent from the clients so that different receivers in the chat room can receive the messages synchrnously. 

Besides using the message_queue, we also use mutex and semaphore to synchronize these section. 
In room.cpp, we use mutex to prevent multiple programs from accessing the same objects at the same time. 
In message_queue.cpp we use semaphore and mutex together since the semaphore allows senders to notify receivers when a message is enqueued, and the receivers
can dequeue the message. This solves the concurrency issue.  
The mutex is used to ensure that only one sender updates the semaphore at a time, so they will not access the information at the same time. 
In addition, a guard object is used to lock and unlock the mutex so that we can prevent synchronization hazards such as deadlocks.
This ensures that the lock is released when it goes out of scope without requiring explicit calls to pthread_mutex_unlock.