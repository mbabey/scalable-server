#ifndef SCALABLE_CLIENT_STATE_H
#define SCALABLE_CLIENT_STATE_H

#include <netinet/in.h>
#include <dc_env/env.h>
#include <dc_error/error.h>

#define MAX_CONNS 500

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
    int accepted_fds[MAX_CONNS];
    int num_conns;
    bool started;
    int wait_period_sec;
};

/**
 * init_state
 * <p>
 * initialize the state object and open a non-blocking TCP socket for listening.
 * </p>
 * @param wait_period_sec load test length in seconds.
 * @param listen_port the port to listen on.
 * @param s pointer the state object to initialize.
 * @param err pointer to a dc_error struct.
 * @param env pointer to a dc_env struct.
 * @return 0 on success. On failure, -1 and set errno.
 */
int init_state(int wait_period_sec, const char *listen_port, struct state * s, struct dc_error * err, struct dc_env * env);

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
