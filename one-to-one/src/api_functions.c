#include "../../api_functions.h"
#include "../include/objects.h"

#include <mem_manager/manager.h>
#include <stdio.h>

int initialize_server(struct core_object *co)
{
    printf("INIT ONE-TO-ONE SERVER\n");
    
    co->so = (struct state_object *) Mmm_calloc(1, sizeof (struct state_object), co->mm);
    if (co->so == NULL)
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
