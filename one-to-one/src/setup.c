#include "../include/objects.h"
#include "../include/setup.h"



struct state_object *setup_state(struct memory_manager *mm)
{
    struct state_object *so;
    
    so = (struct state_object *) Mmm_calloc(1, sizeof (struct state_object), mm);
    if (!so)
    {
        return NULL;
    }
    
    so->listen_addr = (struct sockaddr_in *) Mmm_calloc(1, sizeof(struct sockaddr_in), mm);
    
    
    
    return so;
}


