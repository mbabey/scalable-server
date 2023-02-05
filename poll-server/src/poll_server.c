#include "../include/objects.h"
#include "../include/poll_server.h"

#include <errno.h>
#include <sys/socket.h> // back compatability
#include <sys/types.h>  // back compatability
#include <unistd.h>

struct state_object *setup_state(struct memory_manager *mm)
{
    struct state_object *so;
    
    so = (struct state_object *) Mmm_calloc(1, sizeof(struct state_object), mm);
    if (!so) // Depending on whether more is added to this state object, this if clause may go.
    {
        return NULL;
    }
    
    return so;
}

int open_server_for_listen(struct state_object *so, struct sockaddr_in *listen_addr)
{
    int fd;
    
    fd = socket(PF_INET, SOCK_STREAM, 0);
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
    so->listen_fd = fd;
    
    return 0;
}

int destroy_state(struct state_object *so)
{
    int status;
    int big_bad_error;
    
    big_bad_error = 0;
    status        = close(so->listen_fd);
    if (status == -1)
    {
        switch (errno)
        {
            case EBADF: // Not a problem.
            {
                errno = 0;
                break;
            }
            default:
            {
                big_bad_error = 1; // TODO: EIO or EINTR; not sure what to do here.
            }
        }
    }
    
    for (size_t sfd_num = 0; sfd_num < MAX_CONNECTIONS; ++sfd_num)
    {
        status = close(*(so->client_fd + sfd_num));
        if (status == -1)
        {
            switch (errno)
            {
                case EBADF: // Not a problem.
                {
                    errno = 0;
                    break;
                }
                default:
                {
                    big_bad_error = 1; // TODO: EIO or EINTR; not sure what to do here.
                }
            }
        }
    }
    
    if (big_bad_error)
    {
        return -1;
    }
    
    return 0;
}
