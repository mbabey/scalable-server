#ifndef SCALABLE_SERVER_ONETOONE_OBJECTS_H
#define SCALABLE_SERVER_ONETOONE_OBJECTS_H

#include "../../core/include/objects.h"

struct state_object {
    int listen_fd;
    int client_fd;
    struct sockaddr_in client_addr;
};

#endif //SCALABLE_SERVER_ONETOONE_OBJECTS_H
