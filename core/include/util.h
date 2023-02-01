#ifndef SCALABLE_SERVER_UTIL_H
#define SCALABLE_SERVER_UTIL_H

#include <dc_c/dc_stdio.h>
#include <mem_manager/manager.h>
#include <netinet/in.h>
#include <sys/types.h>

/**
 * open_file
 * <p>
 * Open a file with a given mode.
 * </p>
 * @param file_name the log file to open.
 * @param mode the mode to open the file with.
 * @return The file. NULL and set errno on failure.
 */
FILE *open_file(const char * file_name, const char * mode);

/**
 * assemble_listen_addr
 * <p>
 * Assemble a the server's listen addr. Zero memory and fill fields.
 * </p>
 * @param listen_addr the address to assemble
 * @param port_num the port number
 * @param ip_addr the IP address
 * @param mm the memory manager object
 * @return 0 on success, -1 and set errno on failure.
 */
int assemble_listen_addr(struct sockaddr_in *listen_addr, const in_port_t port_num, const char *ip_addr);

/**
 * open_lib
 * <p>
 * Open a dynamic library in a given mode.
 * </p>
 * @param lib_name name of the library to open.
 * @param mode the mode to open the library with.
 * @return The library. NULL and set errno on failure.
 */
void *open_lib(const char *lib_name, int mode);

/**
 * close_lib
 * <p>
 * Close a dynamic library.
 * </p>
 * @param lib the library to close.
 * @return 0 on success. On failure, -1 and set errno.
 */
int close_lib(void * lib);

/**
 * get_func
 * <p>
 * Get a function from a dynamic library. Function needs to be cast to the appropriate pointer.
 * </p>
 * @param lib the library to get from.
 * @param func_name the name of the function to get.
 * @return The function. NULL and set errno on failure.
 */
void *get_func(void *lib, const char *func_name);

#endif //SCALABLE_SERVER_UTIL_H
