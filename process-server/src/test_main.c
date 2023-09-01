#include "../../core/include/util.h"

#include <dc_error/error.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char **argv)
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
    
    char *end;
    
    next_state = setup_core_object(&co, env, err, strtol(argv[2], &end, 10), argv[1]);
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
                pid_t pid;
                
                pid = getpid();
                // NOLINTNEXTLINE(concurrency-mt-unsafe) : No threads here
                (void) fprintf(stderr, "Fatal: error during server %d runtime: %d: %s\n",
                               pid, errno, strerror(errno));
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
