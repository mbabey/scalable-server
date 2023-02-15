#ifndef SCALABLE_SERVER_PROCESS_OBJECTS_H
#define SCALABLE_SERVER_PROCESS_OBJECTS_H

#include "../../core/include/objects.h"
#include <semaphore.h>

/**
 * The number of worker processes to be spawned to handle network requests.
 */
#define NUM_CHILD_PROCESSES 4 // Should be a power of 2 for magic indexing purposes.

/**
 * The maximum number of connections that can be accepted by the process server.
 */
#define MAX_CONNECTIONS 5

/**
 * The number of connections that can be queued on the listening socket.
 */
#define CONNECTION_QUEUE 100

struct state_object
{
    pid_t                child_pid[NUM_CHILD_PROCESSES];
    sem_t                pipe_sem_r;
    sem_t                pipe_sem_w;
    sem_t                log_sem;
    int                  domain_fd;
    struct child_struct  *child;
    struct parent_struct *parent;
    struct sockaddr_in   listen_addr;
};

struct child_struct
{
    int                parent_relative_fd;
    int                client_fd;
    struct sockaddr_in client_addr;
};

struct parent_struct
{
    int listen_fd;
};

#endif //SCALABLE_SERVER_PROCESS_OBJECTS_H
