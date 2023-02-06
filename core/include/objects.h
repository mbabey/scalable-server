#ifndef SCALABLE_SERVER_OBJECTS_H
#define SCALABLE_SERVER_OBJECTS_H

#include <netinet/in.h>
#include <stdio.h>

/**
 * core_object
 * <p>
 * Holds the core information for the execution of the framework, regardless
 * of the library loaded. Includes dc_env, dc_error, memory_manager, log file,
 * and state_object. state_object contains library-dependent data, and will be
 * assigned and handled by the loaded library.
 * </p>
 */
struct core_object {
    const struct dc_env *env;
    struct dc_error *err;
    struct memory_manager *mm;
    FILE *log_file;
    struct sockaddr_in listen_addr;
    struct state_object *so;
};

#endif //SCALABLE_SERVER_OBJECTS_H
