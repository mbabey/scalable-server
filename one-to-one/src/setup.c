#include "../include/objects.h"
#include "../include/setup.h"

struct state_object *setup_state(struct core_object *co)
{
    struct state_object *so;
    
    so = (struct state_object *) Mmm_calloc(1, sizeof (struct state_object), co->mm);
    if (!so)
    {
        return NULL;
    }
    
    
    
    return so;
}


