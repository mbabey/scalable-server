#ifndef CLIENT_THREAD_H
#define CLIENT_THREAD_H

#include <state.h>

int start_threads(struct state * s, struct dc_error * err, struct dc_env * env);

int stop_threads(struct state * s, struct dc_error * err, struct dc_env * env);

#endif //CLIENT_THREAD_H
