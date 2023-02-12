#ifndef CLIENT_THREAD_H
#define CLIENT_THREAD_H

#include <state.h>

/**
 * start_threads
 * <p>
 * starts a group of threads that run a handle function.
 * </p>
 * @param s pointer to the state object.
 * @param err pointer to the dc_error struct.
 * @param env pointer to the dc_env struct.
 * @return 0 on success. -1 and set errno on failure.
 */
int start_threads(struct state * s, struct dc_error * err, struct dc_env * env);

/**
 * stop_threads
 * <p>
 * waits for threads to finish and stops them.
 * </p>
 * @param err pointer to the dc_error struct.
 * @param env pointer to the dc_env struct.
 * @return 0 on success. -1 and set errno on failure.
 */
int stop_threads(struct dc_error * err, struct dc_env * env);

#endif //CLIENT_THREAD_H
