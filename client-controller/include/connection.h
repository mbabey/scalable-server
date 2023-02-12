#ifndef CLIENT_CONTROLLER_CONNECTION_H
#define CLIENT_CONTROLLER_CONNECTION_H

#include <state.h>

/**
 * send_start
 * <p>
 * send the start command to all connected clients. Command is sent to all connections regardless of any errors.
 * </p>
 * @param s pointer the state object.
 * @param err pointer to a dc_error struct.
 * @param env pointer to a dc_env struct.
 * @return 0 on success. On failure, -1 and set errno.
 */
int send_start(struct state * s, struct dc_error * err, struct dc_env * env);

/**
 * send_stop
 * <p>
 * send the stop command to all connected clients. Command is sent to all connections regardless of any errors.
 * </p>
 * @param s pointer the state object.
 * @param err pointer to a dc_error struct.
 * @param env pointer to a dc_env struct.
 * @return 0 on success. On failure, -1 and set errno.
 */
int send_stop(struct state * s, struct dc_error * err, struct dc_env * env);

#endif //CLIENT_CONTROLLER_CONNECTION_H
