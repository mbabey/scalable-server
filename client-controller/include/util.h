#ifndef CLIENT_CONTROLLER_UTIL_H
#define CLIENT_CONTROLLER_UTIL_H

#include <netinet/in.h>

/**
 * TCP_socket
 * <p>
 * create a TCP socket.
 * </p>
 * @param dst where to assign the socket file descriptor.
 * @return 0 on success. On failure, -1 and set errno.
 */
int TCP_socket(int *dst);

/**
 * init_addr
 * <p>
 * construct a socket address.
 * </p>
 * @param addr where to construct the address.
 * @param port the port to use.
 * @return 0 on success. On failure, -1 and set errno.
 */
int init_addr(struct sockaddr_in *addr, in_port_t port);

/**
 * parse_port
 * <p>
 * parse a port from a string.
 * </p>
 * @param buff the string to parse.
 * @param radix the base to use.
 * @return port on success. On failure, 0 and set errno.
 */
in_port_t parse_port(const char *buff, int radix);

#endif //CLIENT_CONTROLLER_UTIL_H