#ifndef SCALABLE_SERVER_API_FUNCTIONS_H
#define SCALABLE_SERVER_API_FUNCTIONS_H

#include "./core/include/objects.h"

/**
 * Server_States
 * <p>
 * Enumerated states for the server. As return values, indicate the next state the server should take.
 * </p>
 */
enum Server_States {
    INITIALIZE_SERVER = 0,
    RUN_SERVER,
    CLOSE_SERVER,
    ERROR,
    EXIT
};

/**
 * api
 * <p>
 * Definition of an API function pointer.
 * </p>
 */
typedef int (*api) (struct core_object * co);

/**
 * initialize_server
 * <p>
 * Initialize the state object and set up the server to listen for connections.
 * Fill fields in the core_object, open the listening socket, and set up the state.
 * </p>
 * @param co the core data object
 * @return RUN_SERVER. Set errno and return ERROR on failure.
 */
int initialize_server(struct core_object *co);

/**
 * run_server
 * <p>
 * Run the server. Accept new connections, handle messages, and close connections
 * as necessary.
 * </p>
 * @param co the core data object
 * @return CLOSE_SERVER on success. Set errno and return ERROR on failure.
 */
int run_server(struct core_object *co);

/**
 * close_server
 * <p>
 * Destroy the state object. Free memory, terminate child processes, and close open files.
 * </p>
 * @param co the core object
 * @return EXIT.
 */
int close_server(struct core_object *co);

#endif //SCALABLE_SERVER_API_FUNCTIONS_H
