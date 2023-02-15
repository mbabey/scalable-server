#include "../../api_functions.h"
#include "../include/process_server.h"

#include <dc_env/env.h>

int initialize_server(struct core_object *co)
{
    DC_TRACE(co->env);
    printf("INIT POLL SERVER\n");
    
    co->so = setup_process_state(co->mm);
    if (!co->so)
    {
        return ERROR;
    }
    
    if (open_process_server_for_listen(co, co->so, &co->listen_addr) == -1)
    {
        return ERROR;
    }
    
    return RUN_SERVER;
}

int run_server(struct core_object *co)
{
    DC_TRACE(co->env);
    printf("RUN POLL SERVER\n");
    
    if (run_process_server(co) == -1)
    {
        return ERROR;
    }
    
    return CLOSE_SERVER;
}

int close_server(struct core_object *co)
{
    DC_TRACE(co->env);
    printf("CLOSE POLL SERVER\n");
    
    destroy_process_state(co, co->so);
    
    return EXIT;
}
