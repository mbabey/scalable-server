#ifndef SCALABLE_SERVER_PROCESS_OBJECTS_H
#define SCALABLE_SERVER_PROCESS_OBJECTS_H

#include "../../core/include/objects.h"
#include <semaphore.h>

/**
 * The number of worker processes to be spawned to handle network requests.
 */
#define NUM_CHILD_PROCESSES 4 // Must be a power of 2 for magic indexing purposes. fixme: maybe not...

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
    pid_t                child_pids[NUM_CHILD_PROCESSES];
    int                  read_finished_pipe_fds[2];
    sem_t                read_finished_pipe_sem_r;
    sem_t                read_finished_pipe_sem_w;
    sem_t                log_sem_w;
    int                  domain_fd;
    struct child_struct  *child;
    struct parent_struct *parent;
    struct sockaddr_in   listen_addr;
};

struct child_struct
{
    int                client_fd_parent;
    int                client_fd_local;
    struct sockaddr_in client_addr;
};

struct parent_struct
{
    int                listen_fd;
    int                client_fds[MAX_CONNECTIONS];
    struct sockaddr_in client_addrs[MAX_CONNECTIONS];
    size_t             num_connections;
};

#endif //SCALABLE_SERVER_PROCESS_OBJECTS_H
