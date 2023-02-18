#ifndef SCALABLE_SERVER_PROCESS_OBJECTS_H
#define SCALABLE_SERVER_PROCESS_OBJECTS_H

#include "../../core/include/objects.h"

#include <semaphore.h>

/**
 * The number of worker processes to be spawned to handle network requests.
 */
#define NUM_CHILD_PROCESSES 4 // Must be a power of 2 for magic indexing purposes.

/**
 * For each loop macro for looping over child processes.
 */
#define FOR_EACH_CHILD_c for (size_t c = 0; c < NUM_CHILD_PROCESSES; ++c)

/**
* The maximum number of connections that can be accepted by the process server.
*/
#define MAX_CONNECTIONS 5

/**
* The number of connections that can be queued on the listening socket.
*/
#define CONNECTION_QUEUE 100

/**
* Read end of child_finished_pipe or read child_finished_semaphore.
*/
#define READ 0

/**
* Write end of child_finished_pipe or read child_finished_semaphore.
*/
#define WRITE 1

/**
* Read semaphore name.
*/
#define READ_SEM_NAME "/r_206a08"

/**
* Write semaphore name.
*/
#define WRITE_SEM_NAME "/w_e37737"

struct state_object
{
    pid_t                child_pids[NUM_CHILD_PROCESSES];
    int                  c_to_p_pipe_fds[2];
    sem_t                *c_to_f_pipe_sems[2];
    sem_t                *log_sem_w;
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
