#include "../../core/include/util.h"

#include <dc_error/error.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_PORT_BUT_A_NUMBER 5000

int main(void)
{
    int                next_state;
    int                run;
    struct core_object co;
    struct dc_env      *env;
    struct dc_error    *err;
    dc_env_tracer      tracer;
    
    tracer = NULL;
//    tracer = trace_reporter;
    
    err = dc_error_create(true);
    env = dc_env_create(err, false, tracer);
    
    next_state = setup_core_object(&co, env, err, DEFAULT_PORT_BUT_A_NUMBER, DEFAULT_IP);
    if (next_state == -1)
    {
        return EXIT_FAILURE;
    }
    
    run = 1;
    while (run)
    {
        switch (next_state)
        {
            case INITIALIZE_SERVER:
            {
                next_state = initialize_server(&co);
                break;
            }
            case RUN_SERVER:
            {
                next_state = run_server(&co);
                break;
            }
            case CLOSE_SERVER:
            {
                next_state = close_server(&co);
                break;
            }
            case ERROR:
            {
                // NOLINTNEXTLINE(concurrency-mt-unsafe) : No threads here
                (void) fprintf(stderr, "Fatal: error during server runtime: %s\n", strerror(errno));
                next_state = close_server(&co);
                break;
            }
            case EXIT:
            {
                run = 0;
                break;
            }
            default: // Should not get here.
            {
                run = 0;
            }
        }
    }
    
    destroy_core_object(&co);
    
    return next_state;
}
