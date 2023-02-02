#include "../../core/include/util.h"

#include <dc_error/error.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_PORT_BUT_A_NUMBER 5000

int main(void)
{
    int                ret_val;
    struct core_object co;
    struct dc_env      *env;
    struct dc_error    *err;
    dc_env_tracer      tracer;
    void               *lib;
    
    tracer = NULL;
//    tracer = trace_reporter;
    
    err = dc_error_create(true);
    env = dc_env_create(err, false, tracer);
    
    ret_val = setup_core_object(&co, env, err, DEFAULT_PORT_BUT_A_NUMBER, DEFAULT_IP);
    if (ret_val == 0)
    {
        ret_val = initialize_server(&co);
        
        ret_val = run_server(&co);
        
        ret_val = close_server(&co);
        
        destroy_core_object(&co);
    }
    
    return ret_val;
}

if (ret_val != 0)
{
// NOLINTNEXTLINE(concurrency-mt-unsafe) : No threads here
(void) fprintf(stderr, "Fatal: error during server runtime: %s\n", strerror(errno));
}

