#ifndef SCALABLE_CLIENT_STATE_H
#define SCALABLE_CLIENT_STATE_H

#include <dc_env/env.h>
#include <dc_error/error.h>
#include <netinet/in.h>
#include <stdio.h>

struct init_state_params {
    const char *server_ip;
    const char *controller_ip;
    const char *server_port;
    const char *controller_port;
    const char *data_file_name;
    uint16_t wait_period_sec;
};

/**
 * state
 * <p>
 * Struct representing the program state.
 * </p>
 */
struct state {
    in_port_t controller_port;
    const char* controller_ip;
    struct sockaddr_in controller_addr;
    int controller_fd;
    in_port_t server_port;
    char* server_ip;
    char *data;
    off_t data_size;
    uint16_t wait_period_sec;
    bool standalone;
};

/**
 * destroy_state.
 * <p>
 * initialize the state object, open a log file, and connect to controller and server.
 * </p>
 * @param params struct of parameters for initializing the state object.
 * @param s pointer to the state object to initialize.
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
