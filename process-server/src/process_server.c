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
 * c_receive_and_handle_messages
 * <p>
 * Look for action on the domain socket, read the sent client socket, then send the client fd known by the parent
 * through the pipe when reading is done.
 * </p>
 * @param co the core_object
 * @return 0 on success, -1 and set errno on failure
 */
static int c_receive_and_handle_messages(struct core_object *co, struct state_object *so);

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
 * p_toggle_file_descriptors
 * <p>
 * Wait for the read semaphore to be signaled on the child-to-parent pipe. Invert the fd that is passed in the pipe.
 * </p>
 * @param arg the state object
 * @return NULL
 */
static void *p_toggle_file_descriptors(void *arg);

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
        c_receive_and_handle_messages(co, so);
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
    if (pthread_create(&fd_inverter_thread, NULL, p_toggle_file_descriptors, so) != 0)
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
            if (p_accept_new_connection(co, so, pollfds) == -1)
            {
                return -1;
            }
        } else
        {
            if (p_send_fd_to_child(co, so, pollfds) == -1)
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

static void *p_toggle_file_descriptors(void *arg)
{
    struct state_object *so      = (struct state_object *) arg;
    struct pollfd       *pollfds = so->parent->pollfds;
    int                 fd;
    
    while (GOGO_PROCESS)
    {
        sem_wait(so->c_to_f_pipe_sems[READ]); // TODO: see sem_post at end of p_run_poll_loop
        read(so->c_to_p_pipe_fds[READ], &fd, sizeof(int));
        sem_post(so->c_to_f_pipe_sems[WRITE]);
        
        // Loop across pollfds. When match is found, invert the fd at that position.
        for (size_t p = 1; p <= MAX_CONNECTIONS; ++p)
        {
            if (pollfds[p].fd == fd)
            {
                pollfds[p].fd = pollfds[p].fd * -1;
            }
        }
    }
    
    return NULL;
}

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
    (void) fprintf(stdout, "Client connected from %s:%d\n", inet_ntoa(so->client_addr[conn_index].sin_addr),
                   ntohs(so->client_addr[conn_index].sin_port));
    
    return 0;
}

static int c_receive_and_handle_messages(struct core_object *co, struct state_object *so)
{
    DC_TRACE(co->env);
    // TODO: in this function the child process will look for action on the domain socket, read a socket, then send the parent fd through the pipe when reading is done.
    
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
