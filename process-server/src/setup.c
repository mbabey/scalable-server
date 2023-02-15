#include "../include/setup.h"

#include <dc_env/env.h>
#include <mem_manager/manager.h>
#include <unistd.h>

struct state_object *setup_process_state(struct memory_manager *mm)
{
    struct state_object *so;
    
    so = (struct state_object *) Mmm_calloc(1, sizeof(struct state_object), mm);
    if (!so) // Depending on whether more is added to this state object, this if clause may go.
    {
        return NULL;
    }
    
    return so;
}

int open_domain_socket_setup_semaphores(struct core_object *co, struct state_object *so)
{
    // TODO: open the domain socket
    
    // TODO: setup all the semaphores: r/w for child-parent pipe, and w for log
    
    return 0;
}

static int p_open_process_server_for_listen(struct core_object *co, struct parent_struct *parent, struct sockaddr_in *listen_addr)
{
    DC_TRACE(co->env);
    int fd;
    
    fd = socket(PF_INET, SOCK_STREAM, 0); // NOLINT(android-cloexec-socket): SOCK_CLOEXEC dne
    if (fd == -1)
    {
        return -1;
    }
    
    if (bind(fd, (struct sockaddr *) listen_addr, sizeof(struct sockaddr_in)) == -1)
    {
        (void) close(fd);
        return -1;
    }
    
    if (listen(fd, CONNECTION_QUEUE) == -1)
    {
        (void) close(fd);
        return -1;
    }
    
    /* Only assign if absolute success. listen_fd == 0 can be used during teardown
     * to determine whether there is a socket to close. */
    parent->listen_fd = fd;
    
    return 0;
}
