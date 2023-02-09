#ifndef CLIENT_UTIL_H
#define CLIENT_UTIL_H

#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>

/**
 * init_connection
 * <p>
 * connect to a host.
 * </p>
 * @param sock_fd socket to connect with.
 * @param addr host address.
 * @return
 */
int init_connection(int sock_fd, struct sockaddr_in *addr);

/**
 * open_file
 * <p>
 * open a file with a given mode.
 * </p>
 * @param dst where to assign the opened file.
 * @param file_name the file to open.
 * @param mode the mode to open the file with.
 * @return 0 on success. On failure, -1 and set errno.
 */
int open_file(FILE **dst, const char * file_name, const char * mode);

/**
 * set_sock_blocking
 * <p>
 * sets the blocking mode of a socket.
 * </p>
 * @param fd file descriptor of the socket.
 * @param blocking whether the socket should block or not.
 * @return 0 on success. On failure, -1 and set errno.
 */
int set_sock_blocking(int fd, bool blocking);

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
int init_addr(struct sockaddr_in *addr, const char *ip, in_port_t port);

/**
 * parse_port
 * <p>
 * parse a port from a string.
 * </p>
 * @param dst where to assign the parsed port.
 * @param buff the string to parse.
 * @param radix the base to use.
 * @return 0 on success. On failure, -1 and set errno.
 */
int parse_port(in_port_t *dst, const char *buff, int radix);

#endif //CLIENT_UTIL_H
