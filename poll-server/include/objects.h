#ifndef SCALABLE_SERVER_POLL_OBJECTS_H
#define SCALABLE_SERVER_POLL_OBJECTS_H

#include "../../core/include/objects.h"

#define MAX_CONNECTIONS 5

struct state_object {
    int listen_fd;
    int client_fd[MAX_CONNECTIONS];
    struct sockaddr_in *client_addr[MAX_CONNECTIONS];
};

#endif //SCALABLE_SERVER_POLL_OBJECTS_H
