#include "../../api_functions.h"
#include "../include/one_to_one.h"

#include <stdio.h>

int initialize_server(struct core_object *co)
{
    printf("INIT ONE-TO-ONE SERVER\n");
    
    co->so = setup_state(co->mm);q
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
    printf("RUN ONE-TO-ONE SERVER\n");
    
    return 0;
}

int close_server(struct core_object *co)
{
    printf("CLOSE ONE-TO-ONE SERVER\n");
    
    destroy_state(co->so);
    
    return 0;
}
