#include "../include/objects.h"
#include "../include/poll_server.h"

#include <errno.h>
#include <poll.h>
#include <string.h>
#include <sys/socket.h> // back compatability
#include <sys/types.h>  // back compatability
#include <unistd.h>

/**
 * Whether the poll loop should be running.
 */
volatile int GOGO_POLL = 1;

/**
 * execute_poll
 * <p>
 * Execute the poll function to listen for action of pollfds. Action on the
 * listen fd will call accept; otherwise, action will call read all.
 * </p>
 * @param co the core object
 * @param pollfds the pollfd array
 * @return 0 on success, -1 and set errno on failure
 */
int execute_poll(struct core_object *co, struct pollfd *pollfds, nfds_t nfds);

/**
 * poll_accept
 * <p>
 * Accept a new connection to the server. Set the value of the fd
 * increment the num connections in the state object and. Log the connection
 * in the log file.
 * </p>
 * @param co the core object
 * @return the 0 on success, -1 and set errno on failure
 */
int poll_accept(struct core_object *co);

/**
 * poll_comm
 * <p>
 * Read from all file descriptors in pollfds for which POLLIN is set.
 * Remove all file descriptors in pollfds for which POLLHUP is set.
 * </p>
 * @param co the core object
 * @param pollfds the pollfds array
 * @return 0 on success, -1 and set errno on failure
 */
int poll_comm(struct core_object *co, struct pollfd **pollfds);

/**
 * poll_read
 * <p>
 * Read from a file descriptor. Log the results in the log file.
 * </p>
 * @param co the core object // TODO(max): this may be FILE *?
 * @param fd the file descriptor
 * @return 0 on success, -1 on failure and set errno
 */
int poll_read(struct core_object *co, struct pollfd *fd);

/**
 * poll_remove_connection
 * <p>
 * </p>
 * @param co
 * @param fd
 * @return
 */
int poll_remove_connection(struct core_object *co, struct pollfd *fd);

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

int run_poll_server(struct core_object *co)
{
    struct pollfd pollfds[MAX_CONNECTIONS + 1]; // +1 for the listen socket.
    struct pollfd listen_pollfd;
    
    // Set up the listen socket pollfd
    listen_pollfd.fd     = co->so->listen_fd;
    listen_pollfd.events = POLLIN;
    
    memset(pollfds, 0, sizeof(pollfds));
    
    pollfds[0] = listen_pollfd;
    
    if (execute_poll(co, pollfds, sizeof(pollfds)) == -1)
    {
        return -1;
    }
    
    return 0;
}

int execute_poll(struct core_object *co, struct pollfd *pollfds, nfds_t nfds)
{
    int poll_status;
    
    while (GOGO_POLL) // TODO(max): setup signal handler
    {
        poll_status = poll(pollfds, nfds, 0);
        if (poll_status == -1)
        {
            return -1;
        }
        
        // If action on the listen socket.
        if ((*pollfds).revents == POLLIN && co->so->num_connections < MAX_CONNECTIONS)
        {
            if (poll_accept(co) == -1)
            {
                return -1;
            }
        } else
        {
            poll_comm(co, &pollfds);
        }
    }
    
    return 0;
}

int poll_accept(struct core_object *co)
{
    
    
    return 0;
}

int poll_comm(struct core_object *co, struct pollfd **pollfds)
{
    struct pollfd *fd;
    
    for (size_t fd_num = 1; fd_num <= co->so->num_connections; ++fd_num)
    {
        fd = *(pollfds + fd_num);
        if (fd->revents == POLLIN)
        {
            if (poll_read(co, fd) == -1)
            {
                return -1;
            }
        } else if (fd->revents == POLLHUP)
        {
            if (poll_remove_connection(co, fd) == -1)
            {
                return -1;
            }
        }
    }
    
    return 0;
}

int poll_read(struct core_object *co, struct pollfd *fd)
{
    
    
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
