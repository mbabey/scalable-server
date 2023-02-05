#include "../../api_functions.h"
#include "../include/poll_server.h"

int initialize_server(struct core_object *co)
{
    printf("INIT POLL SERVER\n");
    
    co->so = setup_state(co->mm);
    if (!co->so)
    {
        return ERROR;
    }
    
    if (open_server_for_listen(co->so, &co->listen_addr) == -1)
    {
        return ERROR;
    }
    
    return RUN_SERVER;
}

int run_server(struct core_object *co)
{
    printf("RUN POLL SERVER\n");
    
    // Accept connections
    // Add new connections to the pollfds array
    // Read fully from connections
    // As read is done, log results
    
    return CLOSE_SERVER;
}

int close_server(struct core_object *co)
{
    printf("CLOSE POLL SERVER\n");
    
    destroy_state(co->so);
    
    return EXIT;
}
