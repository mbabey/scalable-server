#ifndef SCALABLE_CLIENT_STATE_H
#define SCALABLE_CLIENT_STATE_H

#include <netinet/in.h>
#include <dc_env/env.h>
#include <dc_error/error.h>

/**
 * state
 * <p>
 * Struct representing the program state.
 * </p>
 */
struct state {
    in_port_t listen_port;
    struct sockaddr_in listen_addr;
    int listen_fd;
    int *accepted_fds;
    int num_conns;
};

/**
 * init_state
 * <p>
 * initialize the state object and open a TCP socket for listening.
 * </p>
 * @param listen_port the port to listen on.
 * @param s pointer the state object to initialize.
 * @param err pointer to a dc_error struct.
 * @param env pointer to a dc_env struct.
 * @return 0 on success. On failure, -1 and set errno.
 */
int init_state(const char *listen_port, struct state * s, struct dc_error * err, struct dc_env * env);

/**
 * destroy_state.
 * <p>
 * free any dynamic memory from the state object and close any open connections or file descriptors.
 * </p>
 * @param s pointer to the state object to destroy.
 * @param err pointer to a dc_error struct.
 * @param env pointer to a dc_env struct.
 * @return 0 on success. On failure, -1 and set errno.
 */
int destroy_state(struct state * s, struct dc_error * err, struct dc_env * env);

#endif //SCALABLE_CLIENT_STATE_H
