#include "run.h"

#include <thread.h>

int run_state(struct state * s, struct dc_error * err, struct dc_env * env) {
    DC_TRACE(env);
    bool exit;


    // poll on client socket, set appropriate handlers.
}
