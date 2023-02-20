#ifndef CLIENT_HANDLE_H
#define CLIENT_HANDLE_H

#include <netinet/in.h>

struct handle_args {
    struct sockaddr_in server_addr;
    char * data;
    off_t data_size;
    int thread_id;
};

/**
 * handle
 * <>
 * perform a client request to the server. this involves opening a connection on a socket to the server,
 * writing data (char array), and closing the connection once a response is received.
 * server response time is recorded and written to the logging file.
 * once this process has been completed, the thread runs pthread_test_cancel() and may exit.
 * </p>
 * @param handle_args character array to write to the server.
 * @return none.
 */
void * handle(void *handle_args);

#endif //CLIENT_HANDLE_H
