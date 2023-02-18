#include "../include/setup_teardown.h"

#include <dc_env/env.h>
#include <mem_manager/manager.h>
#include <signal.h>
#include <string.h>
#include <sys/semaphore.h>
#include <sys/wait.h>
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

int setup_semaphores(struct state_object *so);

int open_domain_socket_setup_semaphores(struct core_object *co, struct state_object *so)
{
    // TODO: open the domain socket?
    DC_TRACE(co->env);
    
    if (pipe(so->c_to_p_pipe_fds) == -1)
    {
        return -1;
    }
    
    if (setup_semaphores(so) == -1)
    {
        return -1
    }
    
    return 0;
}

int setup_semaphores(struct state_object *so)
{
    sem_t *sem_r;
    sem_t *sem_w;
    sem_t *sem_l;
    
    sem_r = sem_open(READ_SEM_NAME, O_CREAT, S_IRUSR | S_IWUSR, 0);
    sem_w = sem_open(WRITE_SEM_NAME, O_CREAT, S_IRUSR | S_IWUSR, 0);
    sem_l = sem_open(LOG_SEM_NAME, O_CREAT, S_IRUSR | S_IWUSR, 0);
    if (sem_r == SEM_FAILED || sem_w == SEM_FAILED || sem_l == SEM_FAILED)
    {
        sem_close(sem_r);
        sem_close(sem_w);
        sem_close(sem_l);
        return -1;
    }
    so->c_to_f_pipe_sems[READ]  = sem_r;
    so->c_to_f_pipe_sems[WRITE] = sem_w;
    so->log_sem = sem_l;
    
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
    sem_unlink(READ_SEM_NAME);
    sem_unlink(WRITE_SEM_NAME);
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
