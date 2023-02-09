#ifndef CLIENT_HANDLE_H
#define CLIENT_HANDLE_H

/**
 * handle
 * <>
 * perform a client request to the server. this involves opening a connection on a socket to the server,
 * writing data (char array), and closing the connection once a response is received.
 * server response time is recorded and written to the logging file.
 * once this process has been completed, the thread runs pthread_test_cancel() and may exit.
 * </p>
 * @param data character array to write to the server.
 * @return none.
 */
void * handle(void *data);

#endif //CLIENT_HANDLE_H
