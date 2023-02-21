#ifndef SCALABLE_CLIENT_STATE_H
#define SCALABLE_CLIENT_STATE_H

#include <netinet/in.h>
#include <dc_env/env.h>
#include <dc_error/error.h>

#define MAX_CONNS 500

/**
 * init_state_params
 * <p>
 * Struct containing params for initializing the state.
 * </p>
 */
struct init_state_params {
    const char *listen_port;
    const char *server_ip;
    const char *server_port;
    const char *data_file_name;
    int wait_period_sec;
};

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
    const char * server_ip;
    in_port_t server_port;
    int accepted_fds[MAX_CONNS];
    int num_conns;
    char * data;
    off_t data_size;
    bool started;
    int wait_period_sec;
};

/**
 * init_state
 * <p>
 * initialize the state object and open a non-blocking TCP socket for listening.
 * </p>
 * @param params pointer to the init_state_params structure.
 * @param s pointer the state object to initialize.
 * @param err pointer to a dc_error struct.
 * @param env pointer to a dc_env struct.
 * @return 0 on success. On failure, -1 and set errno.
 */
int init_state(struct init_state_params * params, struct state * s, struct dc_error * err, struct dc_env * env);

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
