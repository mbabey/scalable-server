#include "../include/objects.h"
#include "../include/setup.h"

#include <sys/types.h>
#include <unistd.h>

struct state_object *setup_state(struct memory_manager *mm)
{
    struct state_object *so;
    
    so = (struct state_object *) Mmm_calloc(1, sizeof (struct state_object), mm);
    if (!so) // Depending on whether more is added to this state object, this if clause may go.
    {
        return NULL;
    }
    
    return so;
}

int open_server_for_listen(struct core_object *co)
{
    int fd;
    
    fd = socket(PF_INET, SOCK_STREAM, 0);
    if (fd == -1)
    {
        return -1;
    }
    
    if (bind(fd, (struct sockaddr *) &co->listen_addr, sizeof(struct sockaddr_in)) == -1)
    {
        close(fd);
        return -1;
    }
    
    if (listen(fd, CONNECTION_QUEUE) == -1)
    {
        close(fd);
        return -1;
    }
    
    /* Only assign if absolute success. listen_fd == 0 can be used during teardown
     * to determine whether there is a socket to close. */
    co->so->listen_fd = fd;
    
    return 0;
}
