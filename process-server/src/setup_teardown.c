#include "../include/setup_teardown.h"

#include <dc_env/env.h>
#include <mem_manager/manager.h>
#include <signal.h>
#include <string.h>
#include <sys/semaphore.h>
#include <sys/wait.h>
#include <unistd.h>

/**
 * open_semaphores
 * <p>
 * Open the read, write, and log semaphores. If an error occurs opening any one of them, close them all.
 * </p>
 * @param pipe_sems read/write pipe semaphores
 * @param log_sem log file semaphore
 * @return 0 on success, set errno and -1 on failure
 */
static int open_semaphores(sem_t **pipe_sems, sem_t *log_sem);

/**
 * open_domain_sockets
 * <p>
 * Open a connected pair of domain sockets.
 * </p>
 * @param domain_fds file descriptor array for domain sockets
 * @return 0 on success, -1 and set errno of failure
 */
static int open_domain_sockets(int **domain_fds);

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

int open_pipe_semaphores_domain_sockets(struct core_object *co, struct state_object *so)
{
    // TODO: open the domain socket?
    DC_TRACE(co->env);
    
    if (pipe(so->c_to_p_pipe_fds) == -1) // Open pipe.
    {
        return -1;
    }
    
    if (open_semaphores(so->c_to_f_pipe_sems, so->log_sem) == -1)
    {
        return -1;
    }
    
    if (open_domain_sockets(so->domain_fds) == -1)
    {
        return -1;
    }
    
    return 0;
}

static int open_semaphores(sem_t **pipe_sems, sem_t *log_sem)
{
    pipe_sems[READ] = sem_open(READ_SEM_NAME, O_CREAT, S_IRUSR | S_IWUSR, 0);
    pipe_sems[WRITE] = sem_open(WRITE_SEM_NAME, O_CREAT, S_IRUSR | S_IWUSR, 0);
    log_sem = sem_open(LOG_SEM_NAME, O_CREAT, S_IRUSR | S_IWUSR, 0);
    if (pipe_sems[READ] == SEM_FAILED || pipe_sems[WRITE] == SEM_FAILED || log_sem == SEM_FAILED)
    {
        // Closing an unopened semaphore will return -1 and set errno = EINVAL, which can be ignored.
        sem_close(pipe_sems[READ]);
        sem_close(pipe_sems[WRITE]);
        sem_close(log_sem);
        return -1;
    }

    return 0;
}

static int open_domain_sockets(int **domain_fds)
{
    
    
    return 0;
}

int
p_open_process_server_for_listen(struct core_object *co, struct parent_struct *parent, struct sockaddr_in *listen_addr)
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

void p_destroy_parent_state(struct core_object *co, struct state_object *so, struct parent_struct *parent)
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
    
    close_fd_report_undefined_error(so->c_to_p_pipe_fds[READ], "state of pipe read is undefined.");
    close_fd_report_undefined_error(so->domain_fd, "state of domain socket is undefined.");
    close_fd_report_undefined_error(parent->listen_fd, "state of listen socket is undefined.");
    
    for (size_t sfd_num = 0; sfd_num < MAX_CONNECTIONS; ++sfd_num)
    {
        close_fd_report_undefined_error(*(parent->client_fds + sfd_num), "state of client socket is undefined.");
    }
    
    co->mm->mm_free(co->mm, parent);
    
    sem_close(so->c_to_f_pipe_sems[READ]);
    sem_close(so->c_to_f_pipe_sems[WRITE]);
    sem_close(so->log_sem);
    sem_unlink(READ_SEM_NAME);
    sem_unlink(WRITE_SEM_NAME);
    sem_unlink(LOG_SEM_NAME);
}

void c_destroy_child_state(struct core_object *co, struct state_object *so, struct child_struct *child)
{
    close_fd_report_undefined_error(so->c_to_p_pipe_fds[WRITE], "state of pipe write is undefined.");
    
    co->mm->mm_free(co->mm, child);
}

void close_fd_report_undefined_error(int fd, const char *err_msg)
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
