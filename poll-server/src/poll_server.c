#include "../include/objects.h"
#include "../include/poll_server.h"

#include <arpa/inet.h>
#include <dc_env/env.h>
#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <string.h>
#include <sys/socket.h> // back compatability
#include <sys/types.h>  // back compatability
#include <time.h>
#include <unistd.h>


// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables): must be non-const
/**
 * Whether the poll loop should be running.
 */
volatile int GOGO_POLL = 1;
// NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)

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
 * setup_signal_handler
 * @param sa sigaction struct to fill
 * @return 0 on success, -1 and set errno on failure
 */
static int setup_signal_handler(struct sigaction *sa, int signal);

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
 * get_conn_index
 * <p>
 * Find an index in the file descriptor array where file descriptor == 0.
 * </p>
 * @param client_fds the file descriptor array
 * @return the first index where file descriptor == 0
 */
static int get_conn_index(const int *client_fds);

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
 * poll_recv_and_log
 * <p>
 * Read from a file descriptor. Log the results in the log file.
 * </p>
 * @param co the core object
 * @param pollfd the file descriptor
 * @return 0 on success, -1 on failure and set errno
 */
static int poll_recv_and_log(struct core_object *co, struct pollfd *pollfd, size_t fd_num);

/**
 * log
 * <p>
 * Log the information from one received message into the log file in Comma Separated Value file format.
 * </p>
 * @param co the core object
 * @param so the state object
 * @param fd_num the file descriptor that was read
 * @param bytes the number of bytes read
 * @param start_time the start time of the read
 * @param end_time the end time of the read
 * @param elapsed_time_granular the elapsed time in seconds
 */
static void log(struct core_object *co, struct state_object *so, size_t fd_num, ssize_t bytes,
                time_t start_time, time_t end_time, double elapsed_time_granular);

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
poll_remove_connection(struct core_object *co, struct state_object *so, struct pollfd *pollfd, size_t conn_index,
                       struct pollfd *listen_pollfd);

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
    listen_pollfd.fd      = co->so->listen_fd;
    listen_pollfd.events  = POLLIN;
    listen_pollfd.revents = 0;
    pollfds_len = sizeof(pollfds) / sizeof(*pollfds);
    
    memset(pollfds, 0, sizeof(pollfds));
    
    pollfds[0] = listen_pollfd;
    
    // Set up the headers for the log file.
    (void) fprintf(co->log_file,
                   "connection index,file descriptor,ipv4 address,port number,bytes read,start timestamp,end timestamp,elapsed time (s)\n");
    
    if (execute_poll(co, pollfds, pollfds_len) == -1)
    {
        return -1;
    }
    
    return 0;
}

static int execute_poll(struct core_object *co, struct pollfd *pollfds, nfds_t nfds)
{
    DC_TRACE(co->env);
    int              poll_status;
    struct sigaction sigint;
    
    if (setup_signal_handler(&sigint, SIGINT) == -1)
    {
        return -1;
    }
    if (setup_signal_handler(&sigint, SIGTERM) == -1)
    {
        return -1;
    }
    
    while (GOGO_POLL)
    {
        poll_status = poll(pollfds, nfds, -1);
        if (poll_status == -1)
        {
            return (errno == EINTR) ? 0 : -1;
        }
        
        // If action on the listen socket.
        if ((*pollfds).revents == POLLIN)
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

static int setup_signal_handler(struct sigaction *sa, int signal)
{
    sigemptyset(&sa->sa_mask);
    sa->sa_flags   = 0;
    sa->sa_handler = sigint_handler;
    if (sigaction(signal, sa, 0) == -1)
    {
        return -1;
    }
    return 0;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

static void sigint_handler(int signal)
{
    GOGO_POLL = 0;
}

#pragma GCC diagnostic pop

static int poll_accept(struct core_object *co, struct state_object *so, struct pollfd *pollfds)
{
    DC_TRACE(co->env);
    int       new_cfd;
    size_t    conn_index;
    socklen_t sockaddr_size;
    
    conn_index    = get_conn_index(so->client_fd);
    sockaddr_size = sizeof(struct sockaddr_in);
    
    new_cfd = accept(so->listen_fd, (struct sockaddr *) &so->client_addr[conn_index], &sockaddr_size);
    if (new_cfd == -1)
    {
        return -1;
    }
    
    so->client_fd[conn_index] = new_cfd; // Only save in array if valid.
    pollfds[conn_index + 1].fd     = new_cfd; // Plus one because listen_fd.
    pollfds[conn_index + 1].events = POLLIN;
    ++so->num_connections;
    
    if (so->num_connections >= MAX_CONNECTIONS)
    {
        pollfds->events = 0; // Turn off POLLIN on the listening socket when max connections reached.
    }
    
    // NOLINTNEXTLINE(concurrency-mt-unsafe): No threads here
    (void) fprintf(stdout, "Client connected from %s:%d ; events: %d\n",
                   inet_ntoa(so->client_addr[conn_index].sin_addr),
                   ntohs(so->client_addr[conn_index].sin_port), pollfds[conn_index + 1].events);
    
    return 0;
}

static int get_conn_index(const int *client_fds)
{
    int conn_index = 0;
    
    for (int i = 0; i < MAX_CONNECTIONS; ++i)
    {
        if (*(client_fds + i) == 0)
        {
            conn_index = i;
            break;
        }
    }
    return conn_index;
}

static int poll_comm(struct core_object *co, struct state_object *so, struct pollfd *pollfds)
{
    DC_TRACE(co->env);
    struct pollfd *pollfd;
    
    for (size_t fd_num = 1; fd_num <= MAX_CONNECTIONS; ++fd_num)
    {
        pollfd = pollfds + fd_num;
        if (pollfd->revents == POLLIN)
        {
            if (poll_recv_and_log(co, pollfd, fd_num) == -1)
            {
                return -1;
            }
            // NOLINTNEXTLINE(hicpp-signed-bitwise): never negative
        } else if ((pollfd->revents & POLLHUP) || (pollfd->revents & POLLERR))
            // Client has closed other end of socket.
            // On MacOS, POLLHUP will be set; on Linux, POLLERR will be set.
        {
            (poll_remove_connection(co, so, pollfd, fd_num - 1, pollfds));
        }
        pollfd->revents = 0;
    }
    
    return 0;
}

static int poll_recv_and_log(struct core_object *co, struct pollfd *pollfd, size_t fd_num)
{
    DC_TRACE(co->env);
    ssize_t  bytes;
    char     *buffer;
    size_t   buffer_size;
    uint32_t bytes_to_read;
    uint32_t bytes_read;
    time_t   start_time;
    time_t   end_time;
    clock_t  start_time_granular;
    clock_t  end_time_granular;
    double   elapsed_time_granular;
    double   elapsed_time_granular_total;
    
    // Read the number of bytes that will be sent in the message.
    bytes = recv(pollfd->fd, &bytes_to_read, sizeof(bytes_to_read), 0);
    if (bytes == -1)
    {
        return -1;
    }

    bytes_to_read = ntohl(bytes_to_read);
    
    // Allocate the buffer based on bytes to read.
    buffer_size = (bytes_to_read + 1 * sizeof(char));
    buffer      = (char *) Mmm_malloc(buffer_size, co->mm);
    if (!buffer)
    {
        return -1;
    }
    
    bytes_read                  = 0;
    elapsed_time_granular_total = 0.0;
    start_time                  = time(NULL);
    // Log the start time of the transaction.
    log(co, co->so, fd_num, bytes, start_time, 0, elapsed_time_granular_total);
    while (bytes_read < bytes_to_read && bytes != 0)
    {
        memset(buffer, 0, buffer_size);
        
        start_time_granular = clock();
        bytes               = recv(pollfd->fd, buffer + bytes_read, sizeof(buffer), 0); // Recv into buffer
        if (bytes == -1)
        {
            co->mm->mm_free(co->mm, buffer);
            return -1;
        }
        end_time_granular = clock();
        
        bytes_read += bytes;
        
        // Get the milliseconds per read.
        elapsed_time_granular = (double) (end_time_granular - start_time_granular) / CLOCKS_PER_SEC;
        elapsed_time_granular_total += elapsed_time_granular;
        // Log the number of bytes read and the time per recv.
        log(co, co->so, fd_num, bytes, 0, 0, elapsed_time_granular);
    }
    end_time                    = time(NULL);
    // Log the end time of the transaction.
    log(co, co->so, fd_num, bytes_read, start_time, end_time, elapsed_time_granular_total);
    
    co->mm->mm_free(co->mm, buffer);
    
    bytes_read = htonl(bytes_read);
    bytes      = send(pollfd->fd, &bytes_read, sizeof(bytes_read), 0); // Send back the number of bytes read.
    if (bytes == -1)
    {
        return -1;
    }
    
    return 0;
}

static void log(struct core_object *co, struct state_object *so, size_t fd_num, ssize_t bytes,
                time_t start_time, time_t end_time, double elapsed_time_granular)
{
    size_t    conn_index;
    int       fd;
    char      *client_addr;
    in_port_t client_port;
    char      *start_time_str;
    char      *end_time_str;
    
    // NOLINTBEGIN(concurrency-mt-unsafe): No threads here
    conn_index  = fd_num - 1;
    fd          = so->client_fd[conn_index];
    client_addr = inet_ntoa(so->client_addr[conn_index].sin_addr);
    client_port = ntohs(so->client_addr[conn_index].sin_port);
    if (start_time)
    {
        start_time_str = ctime(&start_time);
        *(start_time_str + strlen(start_time_str) - 1) = '\0'; // Remove newline
    } else
    {
        start_time_str = NULL;
    }
    if (end_time)
    {
        end_time_str = ctime(&end_time);
        *(end_time_str + strlen(end_time_str) - 1) = '\0'; // Remove newline
    } else
    {
        end_time_str = NULL;
    }
    // NOLINTEND(concurrency-mt-unsafe)
    
    /* log the connection index, the file descriptor, the client IP, the client port,
     * the number of bytes read, the start time, and the end time */
    (void) fprintf(co->log_file, "%lu,%d,%s,%d,%lu,%s,%s,%lf\n", conn_index, fd, client_addr, client_port, bytes,
                   (start_time_str) ? start_time_str : "NULL", (end_time_str) ? end_time_str : "NULL",
                   elapsed_time_granular);
}

static void
poll_remove_connection(struct core_object *co, struct state_object *so, struct pollfd *pollfd, size_t conn_index,
                       struct pollfd *listen_pollfd)
{
    DC_TRACE(co->env);
    
    // close the fd
    close_fd_report_undefined_error(pollfd->fd, "state of client socket is undefined.");
    
    // NOLINTNEXTLINE(concurrency-mt-unsafe): No threads here
    (void) fprintf(stdout, "Client from %s:%d disconnected\n", inet_ntoa(so->client_addr[conn_index].sin_addr),
                   ntohs(so->client_addr[conn_index].sin_port));
    
    // zero the pollfd struct, the fd in the state object, and the client_addr in the state object.
    memset(pollfd, 0, sizeof(struct pollfd));
    memset(&so->client_addr[conn_index], 0, sizeof(struct sockaddr_in));
    so->client_fd[conn_index] = 0;
    --so->num_connections;
    
    if (so->num_connections < MAX_CONNECTIONS)
    {
        listen_pollfd->events = POLLIN; // Turn on POLLIN on the listening socket when less than max connections.
    }
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
