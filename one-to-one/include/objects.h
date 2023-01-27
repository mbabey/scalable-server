#ifndef SCALABLE_SERVER_OBJECTS_H
#define SCALABLE_SERVER_OBJECTS_H

#include <netinet/in.h>

struct state_object {
    int listen_fd;
    int client_fd;
    struct sockaddr_in *listen_addr;
    struct sockaddr_in *client_addr;
};

#endif //SCALABLE_SERVER_OBJECTS_H
