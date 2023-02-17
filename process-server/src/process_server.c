#include "../include/objects.h"
#include "../include/process_server.h"
#include "../include/setup.h"

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

/**
 * For each loop macro for looping over child processes.
 */
#define FOR_EACH_CHILD_c for (size_t c = 0; c < NUM_CHILD_PROCESSES; ++c)

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
static int p_run_poll_loop(struct core_object *co, struct state_object *so);

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
 * p_destroy_parent_state
 * <p>
 * Perform actions necessary to close the parent process: signal all child processes to end,
 * close pipe read end, close UNIX socket connection, close active connections, close semaphores,
 * free allocated memory.
 * </p>
 * @param co the core object
 * @param parent the parent struct
 */
static void p_destroy_parent_state(struct core_object *co, struct state_object *so, struct parent_struct *parent);

/**
 * c_destroy_child_state
 * <p>
 * Perform actions necessary to close the child process: close pipe write end, close
 * UNIX socket connection, free allocated memory.
 * </p>
 * @param co the core object
 * @param child the child struct
 */
static void c_destroy_child_state(struct core_object *co, struct state_object *so, struct child_struct *child);

/**
 * close_fd_report_undefined_error
 * <p>
 * Close a file descriptor and report an error which would make the file descriptor undefined.
 * </p>
 * @param fd the fd to close
 * @param err_msg the error message to print
 */
static void close_fd_report_undefined_error(int fd, const char *err_msg);

int setup_process_server(struct core_object *co, struct state_object *so)
{
    DC_TRACE(co->env);
    
    so = setup_process_state(co->mm);
    if (!so)
    {
        return -1;
    }
    
    if (open_domain_socket_setup_semaphores(co, so) == -1)
    {
        return -1;
    }
    
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
        p_open_process_server_for_listen(co, so->parent, &co->listen_addr);
        p_run_poll_loop(co, so);
    } else if (so->child)
    {
        c_receive_and_handle_messages(co, so);
    }
    
    return 0;
}

static int p_run_poll_loop(struct core_object *co, struct state_object *so)
{
    DC_TRACE(co->env);
    // TODO: in this function the server process (parent) will poll active sockets then send them to the children.
    //  if the active socket is the listen socket, the parent will accept a new connection. Very similar to poll server.
    
    // Recommend: use while (GOGO_PROCESS) :) see poll-server/poll_server.c::execute_poll():line 212
    // The function in here will look exactly the same, except the poll_comm will be a function that sends
    // the message to a child process.
    
    return 0;
}

static int c_receive_and_handle_messages(struct core_object *co, struct state_object *so)
{
    DC_TRACE(co->env);
    // TODO: in this function the child process will look for action on the domain socket, read a socket, then send the parent fd through the pipe when reading is done.
    
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

void destroy_process_state(struct core_object *co, struct state_object *so)
{
    DC_TRACE(co->env);
    
    if (so->parent)
    {
        p_destroy_parent_state(co, so, so->parent);
    } else if (so->child)
    {
        c_destroy_child_state(co, NULL, so->child);
    }
}

static void p_destroy_parent_state(struct core_object *co, struct state_object *so, struct parent_struct *parent)
{
    int status;
    
    FOR_EACH_CHILD_c // Send signals to child processes real quick.
    {
        kill(so->child_pids[c], SIGINT);
    }
    FOR_EACH_CHILD_c // Wait for child processes to wrap up.
    {
        waitpid(so->child_pids[c], &status, 0);
    }
    
    close_fd_report_undefined_error(so->child_finished_pipe_fds[READ], "state of pipe read is undefined.");
    close_fd_report_undefined_error(so->domain_fd, "state of domain socket is undefined.");
    close_fd_report_undefined_error(parent->listen_fd, "state of listen socket is undefined.");
    
    for (size_t sfd_num = 0; sfd_num < MAX_CONNECTIONS; ++sfd_num)
    {
        close_fd_report_undefined_error(*(parent->client_fds + sfd_num), "state of client socket is undefined.");
    }
    
    co->mm->mm_free(co->mm, parent);
    
    sem_close(so->child_finished_pipe_sems[READ]);
    sem_close(so->child_finished_pipe_sems[WRITE]);
//    sem_unlink(READ_SEM_NAME);
//    sem_unlink(WRITE_SEM_NAME);
}

static void c_destroy_child_state(struct core_object *co, struct state_object *so, struct child_struct *child)
{
    close_fd_report_undefined_error(so->child_finished_pipe_fds[WRITE], "state of pipe write is undefined.");
    
    co->mm->mm_free(co->mm, child);
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
