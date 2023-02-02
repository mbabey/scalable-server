#ifndef SCALABLE_CLIENT_STATE_H
#define SCALABLE_CLIENT_STATE_H

#include <netinet/in.h>

struct state {
    in_port_t port_in;
    char* ip_in;
    struct sockaddr_in addr_in;
    int fd_in;
    int fd_out;
};

#endif //SCALABLE_CLIENT_STATE_H
