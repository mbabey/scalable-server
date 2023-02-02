#ifndef SCALABLE_SERVER_UTIL_H
#define SCALABLE_SERVER_UTIL_H

#include "../../api_functions.h"
#include "objects.h"

#include <dc_c/dc_stdio.h>
#include <sys/types.h>

#define DEFAULT_LIBRARY "../../one-to-one/cmake-build-debug/libone-to-one.dylib" // TODO: relative path should be changed to absolute.
#define DEFAULT_PORT "5000"
#define DEFAULT_IP "123.123.123.123" // TODO: will need to get the IP address by default

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
 * trace_reporter
 * <p>
 * formatting function for trace reporting.
 * </p>
 * @param env pointer to a dc_env struct
 * @param file_name name of the file the trace occurs in.
 * @param function_name name of the function the trace occurs in.
 * @param line_number the line the trace occurs in.
 */
void trace_reporter(const struct dc_env *env, const char *file_name, const char *function_name, size_t line_number);

/**
 * setup_core_object
 * <p>
 * Zero the core_object. Setup other objects and attach them to the core_object.
 * Open the log file and attach it to the core object.
 * </p>
 * @param co the core object
 * @param env the environment object
 * @param err the error object
 * @param port_num the port number to listen on
 * @param ip_addr the ip address to listen on
 * @return 0 on success. On failure, -1 and set errno.
 */
int setup_core_object(struct core_object *co, const struct dc_env *env, struct dc_error *err, in_port_t port_num,
                      const char *ip_addr);

/**
 * get_api
 * <p>
 * Open a given library and attempt to load API functions into the api_functions struct.
 * </p>
 * @param api struct containing API functions.
 * @param lib_name name of the library.
 * @param env pointer to a dc_env struct.
 * @return The opened library. NULL and set errno on failure.
 */
void *get_api(struct api_functions *api, const char *lib_name, const struct dc_env *env);

/**
 * close_lib
 * <p>
 * Close a dynamic library.
 * </p>
 * @param lib the library to close.
 * @param lib_name the name of the library to close
 * @return 0 on success. On failure, -1 and set errno.
 */
int close_lib(void *lib, const char *lib_name);

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
