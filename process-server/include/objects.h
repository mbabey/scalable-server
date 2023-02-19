#ifndef SCALABLE_SERVER_PROCESS_OBJECTS_H
#define SCALABLE_SERVER_PROCESS_OBJECTS_H

#include "../../core/include/objects.h"

#include <semaphore.h>
#include <poll.h>

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
* Pipe read semaphore name.
*/
#define PIPE_READ_SEM_NAME "/pr_206a08" // Random hex to prevent collision of this filename with others.

/**
* Pipe write semaphore name.
*/
#define PIPE_WRITE_SEM_NAME "/pw_206a08"

/**
* Domain socket read semaphore name.
*/
#define DOMAIN_READ_SEM_NAME "/dr_206a08"

/**
* Domain socket write semaphore name.
*/
#define DOMAIN_WRITE_SEM_NAME "/dw_206a08"

/**
* Log semaphore name.
*/
#define LOG_SEM_NAME "/l_206a08"

struct state_object
{
    pid_t                child_pids[NUM_CHILD_PROCESSES];
    int                  domain_fds[2];
    int                  c_to_p_pipe_fds[2];
    sem_t                *c_to_f_pipe_sems[2];
    sem_t                *domain_sems[2];
    sem_t                *log_sem;
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
    struct pollfd      pollfds[1 + MAX_CONNECTIONS]; // 0th position is the listen socket fd.
    struct sockaddr_in client_addrs[MAX_CONNECTIONS];
    size_t             num_connections;
};

#endif //SCALABLE_SERVER_PROCESS_OBJECTS_H
