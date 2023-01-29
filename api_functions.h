#ifndef SCALABLE_SERVER_API_FUNCTIONS_H
#define SCALABLE_SERVER_API_FUNCTIONS_H

#include "./core/include/objects.h"

/**
 * initialize
 * <p>
 * Initialize the state object and set up the server to listen for connections.
 * Fill fields in the core_object, open the listening socket, and set up the state.
 * </p>
 * @param co the core data object
 * @return the listen socket fd on success. Set errno and return -1 on failure.
 */
int initialize_server(struct core_object *co);

/**
 * run_server
 * <p>
 * Run the server. Accept new connections, handle messages, and close connections
 * as necessary.
 * </p>
 * @param co the core data object
 * @return 0 on success. Set errno and return -1 on failure.
 */
int run_server(struct core_object *co);

/**
 * destroy
 * <p>
 * Destroy the state object. Free memory, terminate child processes, and close open files.
 * </p>
 * @param co the core object
 * @return 0 on success. Set errno and return -1 on failure.
 */
int close_server(struct core_object *co);

#endif //SCALABLE_SERVER_API_FUNCTIONS_H
