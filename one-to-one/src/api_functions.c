#include "../../api_functions.h"
#include "../include/setup.h"

#include <stdio.h>

int initialize_server(struct core_object *co)
{
    printf("INIT ONE-TO-ONE SERVER\n");
    
    co->so = setup_state(co->mm);
    if (!co->so)
    {
        return -1;
    }
    
    if (open_server_for_listen(co) == -1)
    {
        return -1;
    }
    
    return 0;
}

int run_server(struct core_object *co)
{
    printf("RUN ONE-TO-ONE SERVER\n");

    return 0;
}

int close_server(struct core_object *co)
{
    printf("CLOSE ONE-TO-ONE SERVER\n");

    return 0;
}
