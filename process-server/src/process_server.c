#include "../include/objects.h"
#include "../include/process_server.h"
#include "../include/setup_teardown.h"

#include <arpa/inet.h>
#include <dc_env/env.h>
#include <errno.h>
#include <mem_manager/manager.h>
#include <netinet/in.h>
#include <poll.h>
#include <signal.h>
#include <string.h>
#include <sys/socket.h> // back compatability
#include <sys/types.h>  // back compatability
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>
#include <pthread.h>

// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables): must be non-const
/**
 * Whether the loop at the heart of the program should be running.
 */
volatile int GOGO_PROCESS = 1;
// NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)

/**
 * fork_child_processes
 * <p>
 * Fork the main process into the specified number of child processes. Save the child pids. Set the parent
 * struct to NULL in the child and the child struct the NULL in the parent to identify whether a process
 * is a parent or a child,
 * </p>
 * @param co the core object
 * @param so the state object
 * @return 0 on success, -1 and set errno on failure.
 */
static int fork_child_processes(struct core_object *co, struct state_object *so);

/**
 * p_run_poll_loop
 * <p>
 * Run the process server. Wait for activity on one of the processed sockets; if activity
 * is on the listen socket, accept a new connection. If activity is on any other socket,
 * handle that message.
 * </p>
 * @param co the core object
 * @return 0 on success, -1 and set errno on failure
 */
static int p_run_poll_loop(struct core_object *co, struct state_object *so, struct parent_struct *parent);

/**
 * setup_signal_handler
 * @param sa sigaction struct to fill
 * @return 0 on success, -1 and set errno on failure
 */
static int setup_signal_handler(struct sigaction *sa, int signal);

/**
 * end_gogo_handler
 * <p>
 * Handler for SIGINT. Set the running loop conditional to 0.
 * </p>
 * @param signal the signal received
 */
static void end_gogo_handler(int signal);

/**
 * p_watch_pipe_reenable_fds
 * <p>
 * Wait for the read semaphore to be signaled on the child-to-parent pipe. Invert the fd that is passed in the pipe.
 * </p>
 * @param arg the state object
 * @return NULL
 */
static void *p_watch_pipe_reenable_fds(void *arg);

/**
 * p_accept_new_connection
 * <p>
 * Accept a new connection to the server. Set the value of the fd
 * increment the num connections in the state object and. Log the connection
 * in the log file.
 * </p>
 * @param co the core object
 * @param parent the state object
 * @param pollfds the pollfd array
 * @return the 0 on success, -1 and set errno on failure
 */
static int p_accept_new_connection(struct core_object *co, struct parent_struct *parent, struct pollfd *pollfds);

/**
 * p_get_pollfd_index
 * <p>
 * Find an index in the file descriptor array where file descriptor == 0.
 * </p>
 * @param pollfds the file descriptor array
 * @return the first index where file descriptor == 0
 */
static int p_get_pollfd_index(const struct pollfd *pollfds);

/**
 * p_handle_socket_action
 * <p>
 * Read from all file descriptors in pollfds for which POLLIN is set.
 * Remove all file descriptors in pollfds for which POLLHUP is set.
 * </p>
 * @param co the core object
 * @param so the state object
 * @param pollfds the pollfds array
 * @return 0 on success, -1 and set errno on failure
 */
static int p_handle_socket_action(struct core_object *co, struct state_object *so, struct pollfd *pollfds);

/**
 * p_send_to_child
 * <p>
 * Send an active socket over the domain socket to one of the child processes.
 * </p>
 * @param co the core object
 * @param so the state object
 * @param parent the parent struct
 * @param active_pollfd the active socket
 * @param conn_index the index of the pollfd in the fd array.
 * @return 0 on success, -1 and set errno on failure
 */
static int p_send_to_child(struct core_object *co, struct state_object *so, struct pollfd *active_pollfd);

/**
 * p_remove_connection
 * <p>
 * Close a connection and remove the fd from the list of pollfds.
 * </p>
 * @param co the core object
 * @param parent the state object
 * @param pollfd the pollfd to close and clean
 * @param conn_index the index of the connection in the array of client_fds and client_addrs
 * @param listen_pollfd the listen pollfd
 */
static void p_remove_connection(struct core_object *co, struct parent_struct *parent,
                                struct pollfd *pollfd, size_t conn_index, struct pollfd *listen_pollfd);


/**
 * c_run_child_process
 * <p>
 * Set up and run a child process that handles action on a socket passed to it by the server.
 * </p>
 * @param co the core object
 * @param so the state object
 * @return 0 on success, -1 and set errno on failure.
 */
static int c_run_child_process(struct core_object *co, struct state_object *so);

/**
 * c_receive_and_handle_messages
 * <p>
 * Look for action on the domain socket, read the sent client socket, then send the client fd known by the parent
 * through the pipe when reading is done.
 * </p>
 * @param co the core_object
 * @return 0 on success, -1 and set errno on failure
 */
static int c_receive_and_handle_messages(struct core_object *co, struct state_object *so, struct child_struct *child);

/**
 * c_get_file_description_from_domain_socket
 * <p>
 * Wait on the domain socket read semaphore for a file description to be sent on the domain socket. Put the domain
 * socket information into the child struct.
 * </p>
 * @param co the core object
 * @param so the state object
 * @param child the child struct
 * @return 0 on success, -1 and set errno on failure.
 */
static int c_get_file_description_from_domain_socket(struct core_object *co, struct state_object *so,
                                                     struct child_struct *child);

/**
 * c_recv_and_log
 * <p>
 * Receive a message on the socket in the child struct. Log information about the read.
 * </p>
 * @param co the core object
 * @param so the state object
 * @param child the child struct
 * @return 0  on success, -1 and set errno on failure.
 */
static int c_recv_and_log(struct core_object *co, struct state_object *so, struct child_struct *child);

/**
 * c_inform_parent_recv_finished
 * <p>
 * Send the original fd number to the parent over the child-parent pipe. Close the socket on the child.
 * </p>
 * @param co the core object
 * @param so the state object
 * @param child the child struct
 * @return 0 on success, -1 and set errno on failure.
 */
static int c_inform_parent_recv_finished(struct core_object *co, struct state_object *so, struct child_struct *child);

int setup_process_server(struct core_object *co, struct state_object *so)
{
    DC_TRACE(co->env);
    
    so = setup_process_state(co->mm);
    if (!so)
    {
        return -1;
    }
    
    if (open_pipe_semaphores_domain_sockets(co, so) == -1)
    {
        return -1;
    }
    
    if (fork_child_processes(co, so) == -1)
    {
        return -1;
    }
    
    return 0;
}

static int fork_child_processes(struct core_object *co, struct state_object *so)
{
    pid_t pid;
    memset(so->child_pids, 1, sizeof(so->child_pids));
    for (size_t c = 0; c < NUM_CHILD_PROCESSES && so->child_pids[c] != 0; ++c)
    {
        pid = fork();
        if (pid == -1)
        {
            return -1; // will go to ERROR state.
        }
        so->child_pids[c] = pid;
        if (pid == 0)
        {
            so->parent = NULL; // Here for clarity; will already be null.
            so->child  = (struct child_struct *) Mmm_calloc(1, sizeof(struct child_struct), co->mm);
            if (!so->child)
            {
                return -1; // Will go to ERROR state in child process.
            }
        }
    }
    if (pid > 0)
    {
        so->parent = (struct parent_struct *) Mmm_calloc(1, sizeof(struct parent_struct), co->mm);
        if (!so->parent)
        {
            return -1;
        }
        so->child = NULL; // Here for clarity; will already be null.
    }
    
    return 0;
}

int run_process_server(struct core_object *co, struct state_object *so)
{
    // TODO: error checking on -1 ret val in this function.
    
    DC_TRACE(co->env);
    
    // In parent, child will be NULL. In child, parent will be NULL. This behaviour can be used to identify if child or parent.
    if (so->parent)
    {
        close_fd_report_undefined_error(so->c_to_p_pipe_fds[WRITE], "state of parent pipe write is undefined.");
        close_fd_report_undefined_error(so->domain_fds[READ], "state of parent domain socket is undefined.");
        p_open_process_server_for_listen(co, so->parent, &co->listen_addr);
        p_run_poll_loop(co, so, so->parent);
    } else if (so->child)
    {
        close_fd_report_undefined_error(so->c_to_p_pipe_fds[READ], "state of child pipe read is undefined.");
        close_fd_report_undefined_error(so->domain_fds[WRITE], "state of child domain socket is undefined.");
        c_run_child_process(co, so);
    }
    
    return 0;
}

static int p_run_poll_loop(struct core_object *co, struct state_object *so, struct parent_struct *parent)
{
    DC_TRACE(co->env);
    struct sigaction sigint;
    pthread_t        fd_inverter_thread;
    int              poll_status;
    struct pollfd    *pollfds;
    nfds_t           nfds;
    
    if (setup_signal_handler(&sigint, SIGINT) == -1)
    {
        return -1;
    }
    if (setup_signal_handler(&sigint, SIGTERM) == -1)
    {
        return -1;
    }
    if (pthread_create(&fd_inverter_thread, NULL, p_watch_pipe_reenable_fds, so) != 0)
    {
        return -1;
    }
    
    pollfds = parent->pollfds;
    nfds    = MAX_CONNECTIONS + 1;
    
    while (GOGO_PROCESS)
    {
        poll_status = poll(pollfds, nfds, -1);
        if (poll_status == -1)
        {
            return (errno == EINTR) ? 0 : -1;
        }
        
        // If action on the listen socket.
        if ((*pollfds).revents == POLLIN)
        {
            if (p_accept_new_connection(co, so->parent, pollfds) == -1)
            {
                return -1;
            }
        } else
        {
            if (p_handle_socket_action(co, so, pollfds) == -1) // Mom named it; can't change, sorry.
            {
                return -1;
            }
        }
    }
    
    sem_post(so->c_to_f_pipe_sems[READ]); // TODO: is this necessary to kick thread out of loop?
    pthread_join(fd_inverter_thread, NULL);
    
    return 0;
}

static int setup_signal_handler(struct sigaction *sa, int signal)
{
    sigemptyset(&sa->sa_mask);
    sa->sa_flags   = 0;
    sa->sa_handler = end_gogo_handler;
    if (sigaction(signal, sa, 0) == -1)
    {
        return -1;
    }
    return 0;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

static void end_gogo_handler(int signal)
{
    GOGO_PROCESS = 0;
}

#pragma GCC diagnostic pop

static void *p_watch_pipe_reenable_fds(void *arg)
{
    struct state_object *so      = (struct state_object *) arg;
    struct pollfd       *pollfds = so->parent->pollfds;
    int                 fd;
    ssize_t bytes_read;
    
    while (GOGO_PROCESS)
    {
        if (sem_wait(so->c_to_f_pipe_sems[READ]) == -1)
        {
            if (errno != EINTR)
            {
                (void) fprintf(stderr, "Error waiting for read sem in pipe listening thread: %s", strerror(errno));
            }
            return NULL;
        }
        
        bytes_read = read(so->c_to_p_pipe_fds[READ], &fd, sizeof(int));
        
        sem_post(so->c_to_f_pipe_sems[WRITE]);
        
        if (bytes_read == -1)
        {
            (void) fprintf(stderr, "Error reading pipe listening thread: %s", strerror(errno));
        }
        
        // Loop across pollfds. When match is found, invert the fd at that position.
        for (size_t p = 1; p <= MAX_CONNECTIONS; ++p)
        {
            if (pollfds[p].fd == fd * -1) // pollfd.fd here is negative.
            {
                pollfds[p].fd = pollfds[p].fd * -1; // Invert pollfd.fd so it will be read from in poll loop.
            }
        }
    }
    
    return NULL;
}

static int p_accept_new_connection(struct core_object *co, struct parent_struct *parent, struct pollfd *pollfds)
{
    DC_TRACE(co->env);
    int       new_cfd;
    size_t    pollfd_index;
    socklen_t sockaddr_size;
    
    pollfd_index  = p_get_pollfd_index(parent->pollfds);
    sockaddr_size = sizeof(struct sockaddr_in);
    
    // pollfds->fd is listen socket.
    new_cfd = accept(pollfds->fd, (struct sockaddr *) &parent->client_addrs[pollfd_index], &sockaddr_size);
    if (new_cfd == -1)
    {
        return -1;
    }
    
    // Only save in array if valid.
    pollfds[pollfd_index].fd     = new_cfd; // Plus one because listen_fd.
    pollfds[pollfd_index].events = POLLIN;
    ++parent->num_connections;
    
    // Don't need to short-circuit here; will only be in this function if listen socket events == POLLIN.
    if (parent->num_connections >= MAX_CONNECTIONS)
    {
        pollfds->events = 0; // Turn off POLLIN on the listening socket when max connections reached.
    }
    
    // NOLINTNEXTLINE(concurrency-mt-unsafe): No threads here
    (void) fprintf(stdout, "Client connected from %s:%d\n", inet_ntoa(parent->client_addrs[pollfd_index].sin_addr),
                   ntohs(parent->client_addrs[pollfd_index].sin_port));
    
    return 0;
}

static int p_get_pollfd_index(const struct pollfd *pollfds)
{
    int conn_index = 0;
    
    for (int i = 0; i < 1 + MAX_CONNECTIONS; ++i)
    {
        if (pollfds[i].fd == 0)
        {
            conn_index = i;
            break;
        }
    }
    return conn_index;
}

static int p_handle_socket_action(struct core_object *co, struct state_object *so, struct pollfd *pollfds)
{
    DC_TRACE(co->env);
    struct pollfd *pollfd;
    
    for (size_t fd_num = 1; fd_num <= MAX_CONNECTIONS; ++fd_num)
    {
        pollfd = pollfds + fd_num;
        if (pollfd->revents == POLLIN)
        {
            if (p_send_to_child(co, so, pollfd) == -1)
            {
                return -1;
            }
            pollfd->fd *= -1; // Disable the pollfd until it is signaled by the child to be re-enabled.
            
            // NOLINTNEXTLINE(hicpp-signed-bitwise): never negative
        } else if ((pollfd->revents & POLLHUP) || (pollfd->revents & POLLERR))
            // Client has closed other end of socket.
            // On MacOS, POLLHUP will be set; on Linux, POLLERR will be set.
        {
            (p_remove_connection(co, so->parent, pollfd, fd_num - 1, pollfds));
        }
        pollfd->revents = 0;
    }
    
    return 0;
}

static int p_send_to_child(struct core_object *co, struct state_object *so, struct pollfd *active_pollfd)
{
    DC_TRACE(co->env);
    ssize_t        bytes_sent;
    struct msghdr  msghdr;
    struct iovec   iovec;
    struct cmsghdr *cmsghdr;
    char           control_buffer[CMSG_SPACE(sizeof(int))]; // Create space for one cmsghdr storing an integer.
    
    memset(&msghdr, 0, sizeof(struct msghdr));
    memset(&iovec, 0, sizeof(struct iovec));
    memset(&control_buffer, 0, sizeof(control_buffer));
    
    iovec.iov_base = &active_pollfd->fd; // The original file descriptor number to send.
    iovec.iov_len  = sizeof(int);
    
    msghdr.msg_iov        = &iovec; // Put the IO vector in the msghdr to send.
    msghdr.msg_iovlen     = 1;
    msghdr.msg_control    = control_buffer; // Put the control buffer (containing the cmsghdr) into the msghdr to send.
    msghdr.msg_controllen = sizeof(control_buffer);
    
    cmsghdr = CMSG_FIRSTHDR(&msghdr);
    if (!cmsghdr)
    {
        return -1;
    }
    
    cmsghdr->cmsg_level = SOL_SOCKET;
    cmsghdr->cmsg_type  = SCM_RIGHTS; // Indicates it is a file description being sent.
    cmsghdr->cmsg_len   = CMSG_LEN(sizeof(int));
    *((int *) CMSG_DATA(cmsghdr)) = active_pollfd->fd; // The file description to send.
    
    if (sem_wait(so->domain_sems[WRITE]) == -1)
    {
        return (errno == EINTR) ? 0 : -1;
    }
    bytes_sent = sendmsg(so->domain_fds[WRITE], &msghdr, 0); // Send the msghdr.
    if (bytes_sent == -1)
    {
        return -1;
    }
    sem_post(so->domain_sems[READ]);
    
    return 0;
}

static void p_remove_connection(struct core_object *co, struct parent_struct *parent,
                                struct pollfd *pollfd, size_t conn_index, struct pollfd *listen_pollfd)
{
    DC_TRACE(co->env);
    
    // close the fd
    close_fd_report_undefined_error(pollfd->fd, "state of client socket is undefined.");
    
    // NOLINTNEXTLINE(concurrency-mt-unsafe): No threads here
    (void) fprintf(stdout, "Client from %s:%d disconnected\n", inet_ntoa(parent->client_addrs[conn_index].sin_addr),
                   ntohs(parent->client_addrs[conn_index].sin_port));
    
    // Zero the pollfd struct, the fd in the state object, and the client_addr in the state object.
    memset(pollfd, 0, sizeof(struct pollfd));
    memset(&parent->client_addrs[conn_index], 0, sizeof(struct sockaddr_in));
    --parent->num_connections;
    
    // Short-circuit prevent double-setting.
    if (listen_pollfd->events != POLLIN && parent->num_connections < MAX_CONNECTIONS)
    {
        listen_pollfd->events = POLLIN; // Turn on POLLIN on the listening socket when less than max connections.
    }
}

static int c_run_child_process(struct core_object *co, struct state_object *so)
{
    DC_TRACE(co->env);
    pid_t            pid;
    struct sigaction sigint;
    
    pid = getpid();
    
    (void) fprintf(stdout, "Child process with pid %d started.\n", pid);
    
    if (setup_signal_handler(&sigint, SIGINT) == -1)
    {
        return -1;
    }
    if (setup_signal_handler(&sigint, SIGTERM) == -1)
    {
        return -1;
    }
    
    if (c_receive_and_handle_messages(co, so, so->child) == -1)
    {
        return -1;
    }
    
    (void) fprintf(stdout, "Child process with pid %d winding down.\n", pid);
    
    return 0;
}

static int c_receive_and_handle_messages(struct core_object *co, struct state_object *so, struct child_struct *child)
{
    DC_TRACE(co->env);
    // TODO: in this function the child process will look for action on the domain socket, read a socket, then send the parent fd through the pipe when reading is done.
    
    while (GOGO_PROCESS)
    {
        // Clean the child struct.
        memset(child, 0, sizeof(struct child_struct));
        
        if (c_get_file_description_from_domain_socket(co, so, child) == -1)
        {
            return -1;
        }
        if (c_recv_and_log(co, NULL, child) == -1)
        {
            return -1;
        }
        if (c_inform_parent_recv_finished(co, so, child) == -1)
        {
            return -1;
        }
    }
    
    return 0;
}

static int c_get_file_description_from_domain_socket(struct core_object *co, struct state_object *so,
        struct child_struct *child)
{
    DC_TRACE(co->env);
    
    ssize_t        bytes_recv;
    struct msghdr  msghdr;
    struct iovec   iovec;
    struct cmsghdr *cmsghdr;
    char           control_buffer[CMSG_SPACE(sizeof(int))]; // Create space for one cmsghdr storing an integer.
    
    memset(&msghdr, 0, sizeof(struct msghdr));
    memset(&iovec, 0, sizeof(struct iovec));
    memset(&control_buffer, 0, sizeof(control_buffer));
    
    iovec.iov_base = &child->client_fd_parent; // The original file descriptor.
    iovec.iov_len  = sizeof(int);
    
    msghdr.msg_iov        = &iovec; // Put the IO vector in the msghdr to receive.
    msghdr.msg_iovlen     = 1;
    msghdr.msg_control    = control_buffer; // Put the control buffer into the msghdr to receive.
    msghdr.msg_controllen = sizeof(control_buffer);
    
    if (sem_wait(so->domain_sems[READ]) == -1) // Wait for the domain socket read semaphore.
    {
        return (errno == EINTR) ? 0 : -1;
    }
    
    bytes_recv = recvmsg(so->domain_fds[READ], &msghdr, 0);
    
    sem_post(so->domain_sems[WRITE]); // Signal the domain socket write semaphore.
    
    if (bytes_recv == -1)
    {
        return -1;
    }
    
    cmsghdr = CMSG_FIRSTHDR(&msghdr);
    child->client_fd_local = *((int *) CMSG_DATA(cmsghdr)); // The file description.
    
    return 0;
}

static int c_recv_and_log(struct core_object *co, struct state_object *so, struct child_struct *child)
{
    // TODO: recv message from socket. Log message information.
    
    
    return 0;
}

static int c_inform_parent_recv_finished(struct core_object *co, struct state_object *so, struct child_struct *child)
{
    DC_TRACE(co->env);
    ssize_t bytes_written;
    
    if (sem_wait(so->c_to_f_pipe_sems[WRITE]) == -1) // Wait for the pipe write semaphore.
    {
        return (errno == EINTR) ? 0 : -1;
    }
    
    bytes_written = write(so->c_to_p_pipe_fds[WRITE], &child->client_fd_parent, sizeof(int));
    
    sem_post(so->c_to_f_pipe_sems[READ]); // Signal the pipe read semaphore.
    
    if (bytes_written == -1)
    {
        return -1;
    }
    
    return 0;
}

void destroy_process_state(struct core_object *co, struct state_object *so)
{
    DC_TRACE(co->env);
    
    if (so->parent)
    {
        p_destroy_parent_state(co, so, so->parent);
    } else if (so->child)
    {
        c_destroy_child_state(co, so, so->child);
    }
}
