#ifndef SCALABLE_CLIENT_STATE_H
#define SCALABLE_CLIENT_STATE_H

#include <netinet/in.h>

struct state {
    in_port_t controller_port;
    char* controller_ip;
    struct sockaddr_in controller_addr;
    int controller_fd;
    in_port_t server_port;
    char* server_ip;
    struct sockaddr_in server_addr;
    int server_fd;
    int log_fd;
};

#endif //SCALABLE_CLIENT_STATE_H
