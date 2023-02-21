#ifndef CLIENT_RUN_H
#define CLIENT_RUN_H

#include <state.h>

/**
 * run_state
 * <p>
 * runs the client, which listens to the controller for a command. when start is received, threads are started
 * which connect to the server, write data, and close the connection once a reply has been received.
 * when the controller sends a stop command, the threads are stopped, and the client exits.
 * </p>
 * @param s pointer to the state object
 * @param err pointer to the dc_error struct.
 * @param env pointer to the dc_env struct.
 * @return 0 on success. -1 and set errno on failure.
 */
int run_state(struct state * s, struct dc_error * err, struct dc_env * env);

#endif //CLIENT_RUN_H
