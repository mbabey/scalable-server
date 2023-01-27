#ifndef SCALABLE_SERVER_OBJECTS_H
#define SCALABLE_SERVER_OBJECTS_H

#include <netinet/in.h>
#include <semaphore.h>

#define

struct state_object
{
    pid_t                child_pid[];
    sem_t                pipe_sem_r;
    sem_t                pipe_sem_w;
    sem_t                log_sem;
    int                  domain_fd;
    struct child_struct  *child;
    struct parent_struct *parent;
};


struct child_struct
{
    int parent_relative_fd;
    int client_fd;
    struct sockaddr_in *client_addr;
};

struct parent_struct
{
    int listen_fd;
    struct sockaddr_in *listen_addr;
};

#endif //SCALABLE_SERVER_OBJECTS_H
