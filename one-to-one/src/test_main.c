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
        struct api_functions api;
        lib = get_api(&api, DEFAULT_LIBRARY, env);
        if (lib == NULL)
        {
            ret_val = -1;
        } else
        {
            // TODO: how do we want to handle return values here?
            ret_val = api.initialize_server(&co);
            // check error
            
            ret_val = api.run_server(&co);
            // check error
            
            ret_val = api.close_server(&co);
            // check error
            
            ret_val = close_lib(lib);
            if (ret_val != 0)
            {
                // NOLINTNEXTLINE(concurrency-mt-unsafe) : No threads here
                (void) fprintf(stderr, "Fatal: could not close lib_name %s: %s\n", DEFAULT_LIBRARY, strerror(errno));
            }
        }
        destroy_core_object(&co);
    }
    
    return ret_val;
}

