#ifndef CLIENT_LOG_H
#define CLIENT_LOG_H

#include <state.h>

/**
 * log info
 * <>
 * log information about the server connection.
 * </p>
 * @param s pointer to the state object.
 * @param err pointer to the dc_error struct.
 * @param env pointer to the dc_env struct.
 * @return 0 on success. -1 and set errno on failure.
 */
int log_info(struct state * s, struct dc_error * err, struct dc_env * env);

#endif //CLIENT_LOG_H
