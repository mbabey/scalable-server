#include "../../api_functions.h"
#include "../include/poll_server.h"

int initialize_server(struct core_object *co)
{
    printf("INIT POLL SERVER\n");
    
    co->so = setup_state(co->mm);
    if (!co->so)
    {
        return -1;
    }
    
    if (open_server_for_listen(co->so, &co->listen_addr) == -1)
    {
        return -1;
    }
    
    return RUN_SERVER;
}

int run_server(struct core_object *co)
{
    return CLOSE_SERVER;
}

int close_server(struct core_object *co)
{
    return EXIT;
}
