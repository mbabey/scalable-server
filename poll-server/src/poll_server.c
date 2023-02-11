#include "../include/objects.h"
#include "../include/poll_server.h"

#include <errno.h>
#include <poll.h>
#include <string.h>
#include <sys/socket.h> // back compatability
#include <sys/types.h>  // back compatability
#include <unistd.h>
#include <dc_env/env.h>
#include <signal.h>
#include <arpa/inet.h>

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
 * @param nfds the number of fds in the array
 * @return 0 on success, -1 and set errno on failure
 */
static int execute_poll(struct core_object *co, struct pollfd *pollfds, nfds_t nfds);

/**w
 * setup_sigint_handler
 * @param sa sigaction struct to fill
 * @return 0 on success, -1 and set errno on failure
 */
static int setup_sigint_handler(struct sigaction *sa);

/**
 * sigint_handler
 * <p>
 * Handler for SIGINT. Set the running loop conditional to 0.
 * </p>
 * @param signal the signal received
 */
static void sigint_handler(int signal);

/**
 * poll_accept
 * <p>
 * Accept a new connection to the server. Set the value of the fd
 * increment the num connections in the state object and. Log the connection
 * in the log file.
 * </p>
 * @param co the core object
 * @param so the state object
 * @return the 0 on success, -1 and set errno on failure
 */
static int poll_accept(struct core_object *co, struct state_object *so, struct pollfd *pollfds);

/**
 * poll_comm
 * <p>
 * Read from all file descriptors in pollfds for which POLLIN is set.
 * Remove all file descriptors in pollfds for which POLLHUP is set.
 * </p>
 * @param co the core object
 * @param so the state object
 * @param pollfds the pollfds array
 * @return 0 on success, -1 and set errno on failure
 */
static int poll_comm(struct core_object *co, struct state_object *so, struct pollfd *pollfds);

/**
 * poll_read
 * <p>
 * Read from a file descriptor. Log the results in the log file.
 * </p>
 * @param co the core object // TODO(max): this may be FILE *?
 * @param pollfd the file descriptor
 * @return 0 on success, -1 on failure and set errno
 */
static int poll_read(struct core_object *co, struct pollfd *pollfd);

/**
 * poll_remove_connection
 * <p>
 * Close a connection and remove the fd from the list of pollfds.
 * </p>
 * @param co the core object
 * @param so the state object
 * @param pollfd the pollfd to close and clean
 * @param conn_index the index of the connection in the array of client_fds and client_addrs
 */
static void
poll_remove_connection(struct core_object *co, struct state_object *so, struct pollfd *pollfd, size_t conn_index);

/**
 * close_fd_report_undefined_error
 * <p>
 * Close a file descriptor and report an error which would make the file descriptor undefined.
 * </p>
 * @param fd the fd to close
 * @param err_msg the error message to print
 */
static void close_fd_report_undefined_error(int fd, const char *err_msg);

struct state_object *setup_poll_state(struct memory_manager *mm)
{
    struct state_object *so;
    
    so = (struct state_object *) Mmm_calloc(1, sizeof(struct state_object), mm);
    if (!so) // Depending on whether more is added to this state object, this if clause may go.
    {
        return NULL;
    }
    
    return so;
}

int open_poll_server_for_listen(struct core_object *co, struct state_object *so, struct sockaddr_in *listen_addr)
{
    DC_TRACE(co->env);
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
    DC_TRACE(co->env);
    struct pollfd pollfds[MAX_CONNECTIONS + 1]; // +1 for the listen socket.
    struct pollfd listen_pollfd;
    size_t        pollfds_len;
    
    // Set up the listen socket pollfd
    listen_pollfd.fd     = co->so->listen_fd;
    listen_pollfd.events = POLLIN;
    listen_pollfd.revents = 0;
    pollfds_len = sizeof(pollfds) / sizeof(*pollfds);
    
    memset(pollfds, 0, sizeof(pollfds));
    
    pollfds[0] = listen_pollfd;
    
    if (execute_poll(co, pollfds, pollfds_len) == -1)
    {
        return -1;
    }
    
    return 0;
}

static int execute_poll(struct core_object *co, struct pollfd *pollfds, nfds_t nfds)
{
    DC_TRACE(co->env);
    int poll_status;
    struct sigaction sig;
    
    setup_sigint_handler(&sig);
    
    while (GOGO_POLL)
    {
        poll_status = poll(pollfds, nfds, -1);
        if (poll_status == -1)
        {
            return -1;
        }
        
        // If action on the listen socket.
        if ((*pollfds).revents == POLLIN && co->so->num_connections < MAX_CONNECTIONS)
        {
            if (poll_accept(co, co->so, pollfds) == -1)
            {
                return -1;
            }
        } else
        {
            if (poll_comm(co, co->so, pollfds) == -1)
            {
                return -1;
            }
        }
    }
    
    return 0;
}

static int setup_sigint_handler(struct sigaction *sa)
{
    sigemptyset(&sa->sa_mask);
    sa->sa_flags   = 0;
    sa->sa_handler = sigint_handler;
    if (sigaction(SIGINT, sa, 0) == -1)
    {
        return -1;
    }
    return 0;
}

// NOLINTBEGIN
static void sigint_handler(int signal)
{
    GOGO_POLL = 0;
}
// NOLINTEND

static int poll_accept(struct core_object *co, struct state_object *so, struct pollfd *pollfds)
{
    DC_TRACE(co->env);
    int       new_cfd;
    size_t    conn_index;
    socklen_t sockaddr_size;
    
    conn_index    = so->num_connections;
    sockaddr_size = sizeof(struct sockaddr_in);
    
    new_cfd = accept(so->listen_fd, (struct sockaddr *) &so->client_addr[conn_index], &sockaddr_size);
    if (new_cfd == -1)
    {
        return -1;
    }
    
    so->client_fd[conn_index] = new_cfd; // Only save in array if valid.
    pollfds[conn_index + 1].fd = new_cfd; // Plus one because listen_fd.
//    pollfds[conn_index + 1].events = POLLIN;
    ++so->num_connections;
    
    (void) fprintf(stdout, "Client connected from %s:%d\n", inet_ntoa(so->client_addr[conn_index].sin_addr), ntohs(so->client_addr[conn_index].sin_port));
    
    return 0;
}

static int poll_comm(struct core_object *co, struct state_object *so, struct pollfd *pollfds)
{
    DC_TRACE(co->env);
    struct pollfd *pollfd;
    
    for (size_t fd_num = 1; fd_num <= co->so->num_connections; ++fd_num)
    {
        pollfd = pollfds + fd_num;
        if (pollfd->revents == POLLIN)
        {
            if (poll_read(co, pollfd) == -1)
            {
                return -1;
            }
        } else if ((pollfd->revents == POLLHUP) || (pollfd->revents == POLLERR))
            // Client has closed other end of socket.
            // On MacOS, POLLHUP will be set; on Linux, POLLERR will be set.
        {
            (poll_remove_connection(co, so, pollfd, fd_num - 1));
        }
    }
    
    return 0;
}

static int poll_read(struct core_object *co, struct pollfd *pollfd)
{
    DC_TRACE(co->env);
    ssize_t bytes;
    char buffer[BUFSIZ * 16];
    
    bytes = read(pollfd->fd, buffer, sizeof(buffer));
    switch (bytes)
    {
        case -1:
        {
            (void) fprintf(stdout, "bytes read: %lu | buffer: \"%s\"\n", bytes, buffer);
            return -1;
        }
        case 0:
        {
            (void) fprintf(stdout, "bytes read: %lu | buffer: \"%s\"\n", bytes, buffer);
            break;
        }
        default:
        {
            (void) fprintf(stdout, "bytes read: %lu | buffer: \"%s\"\n", bytes, buffer);
        }
    }
    
    return 0;
}

static void poll_remove_connection(struct core_object *co, struct state_object *so, struct pollfd *pollfd, size_t conn_index)
{
    DC_TRACE(co->env);
    
    // close the fd
    close_fd_report_undefined_error(pollfd->fd, "state of client socket is undefined.");
    
    // zero the pollfd struct, the fd in the state object, and the client_addr in the state object.
    memset(pollfd, 0, sizeof(struct pollfd));
    memset(&so->client_addr[conn_index], 0, sizeof(struct sockaddr_in));
    so->client_fd[conn_index] = 0;
}

void destroy_poll_state(struct core_object *co, struct state_object *so)
{
    DC_TRACE(co->env);
    
    close_fd_report_undefined_error(so->listen_fd, "state of listen socket is undefined.");
    
    for (size_t sfd_num = 0; sfd_num < MAX_CONNECTIONS; ++sfd_num)
    {
        close_fd_report_undefined_error(*(so->client_fd + sfd_num), "state of client socket is undefined.");
    }

}

static void close_fd_report_undefined_error(int fd, const char *err_msg)
{
    if (close(fd) == -1)
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
                // NOLINTNEXTLINE(concurrency-mt-unsafe) : No threads here
                (void) fprintf(stderr, "Error: %s; %s\n", strerror(errno), err_msg);
            }
        }
    }
    
}
