#ifndef SCALABLE_SERVER_UTIL_H
#define SCALABLE_SERVER_UTIL_H

#include "../../api_functions.h"
#include "objects.h"

#include <dc_c/dc_stdio.h>
#include <mem_manager/manager.h>
#include <netinet/in.h>
#include <sys/types.h>

/**
 * api_functions
 * <p>
 * Struct containing pointers to all API functions.
 * </p>
 */
struct api_functions
{
    api initialize_server;
    api run_server;
    api close_server;
};

/**
 * setup_core_object
 * <p>
 * Zero the core_object. Setup other objects and attach them to the core_object.
 * Open the log file and attach it to the core object.
 * </p>
 * @param co the core object
 * @return 0 on success. On failure, -1 and set errno.
 */
int setup_core_object(struct core_object *co, const struct dc_env *env, struct dc_error *err, in_port_t port_num,
                      const char *ip_addr);

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
int assemble_listen_addr(struct sockaddr_in *listen_addr, in_port_t port_num, const char *ip_addr);

/**
 * get_api
 * <p>
 * Open a given library and attempt to load API functions into the api_functions struct.
 * </p>
 * @param api_functions struct containing API functions.
 * @param lib_name name of the library.
 * @param env pointer to a dc_env struct.
 * @return The opened library. NULL and set errno on failure.
 */
void *get_api(struct api_functions *api, const char *lib_name, const struct dc_env *env);

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
 * get_func
 * <p>
 * Get a function from a dynamic library. Function needs to be cast to the appropriate pointer.
 * </p>
 * @param lib the library to get from.
 * @param func_name the name of the function to get.
 * @return The function. NULL and set errno on failure.
 */
void *get_func(void *lib, const char *func_name);

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
 * destroy_core_object
 * <p>
 * Destroy the core object and all of its fields. Does not destroy the state object;
 * the state object must be destroyed by the library destroy_server function.
 * </p>
 * @param co the core object
 */
void destroy_core_object(struct core_object *co);

#endif //SCALABLE_SERVER_UTIL_H
