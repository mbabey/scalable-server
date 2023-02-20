#ifndef CLIENT_CONTROLLER_UTIL_H
#define CLIENT_CONTROLLER_UTIL_H

#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>

/**
 * write_fully
 * <p>
 * writes data fully to a file descriptor.
 * </p>
 * @param fd file descriptor to write to.
 * @param data data to write.
 * @param size size of data.
 * @return 0 on success. On failure -1 and set errno.
 */
int write_fully(int fd, void * data, size_t size);

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
 * init_addr
 * <p>
 * construct a socket address with a specified IP.
 * </p>
 * @param dst where to construct the address.
 * @param ip the IP address to use.
 * @param port the port to use.
 * @return 0 on success. On failure, -1 and set errno.
 */
int init_addr(struct sockaddr_in *dst, const char * ip, in_port_t port);

/**
 * init_addr_any
 * <p>
 * construct a socket address with any IP.
 * </p>
 * @param dst where to construct the address.
 * @param port the port to use.
 * @return 0 on success. On failure, -1 and set errno.
 */
int init_addr_any(struct sockaddr_in *dst, in_port_t port);

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

#endif //CLIENT_CONTROLLER_UTIL_H
