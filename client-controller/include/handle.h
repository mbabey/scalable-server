//
// Created by Markus Sample on 2023-02-06.
//

#ifndef CLIENT_CONTROLLER_HANDLE_H
#define CLIENT_CONTROLLER_HANDLE_H

#include <state.h>

/**
 * handle
 * <p>
 * main run function. Polls on listen_fd for incoming connections. Any incoming connection is accepted
 * and its socket fd appended to state.accepted_fds. num_conns is also incremented.
 * Stdin is also polled for any user input. If the user enters "start" connected clients will be signaled to start.
 * </p>
 * @param s pointer the state object.
 * @param err pointer to a dc_error struct.
 * @param env pointer to a dc_env struct.
 * @return 0 on success. On failure, -1 and set errno.
 */
int handle(struct state * s, struct dc_error * err, struct dc_env * env);

#endif //CLIENT_CONTROLLER_HANDLE_H
