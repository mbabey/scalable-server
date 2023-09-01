#include "thread.h"

#include <handle.h>
#include <util.h>

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/**
 * get_processors
 * <p>
 * gets the number of processors the system has.
 * </p>
 * @param dst where to store the value.
 * @return 0 on success. -1 and set errno on failure.
 */
static int get_processors(int *dst);

/**
 * create_threads
 * <p>
 * create n threads that are each passed their own copy of state.data and run handle from handle.h.
 * </p>
 * @param n number of threads to create.
 * @param s pointer to the state object holding the data.
 * @param err pointer to the dc_error struct.
 * @param env pointer to the dc_env struct.
 * @return 0 on success. -1 and set errno on failure.
 */
static int create_threads(int n, struct state * s, struct dc_error * err, struct dc_env * env);

static pthread_t * t_ids = NULL; // array of thread IDs
static int n_threads; // number of threads running
static int n_processors; // number of system processors

int start_threads(struct state * s, struct dc_error * err, struct dc_env * env) {
    DC_TRACE(env);
    int result;

    if (get_processors(&n_processors) == -1) return -1;

    result = create_threads(n_processors, s, err, env);
    if (result == -1) {
        stop_threads(err, env);
        return -1;
    }

    return result;
}

static int create_threads(int n, struct state * s, struct dc_error * err, struct dc_env * env) {
    DC_TRACE(env);

    t_ids = malloc(n * sizeof(pthread_t));
    if (t_ids == NULL) {
        perror("malloc thread id array");
        return -1;
    }

    for (int i = 0; i < n; i++) {
        struct handle_args * h_args = malloc(sizeof(struct handle_args));
        if (h_args == NULL) {
            perror("malloc for thread handle args");
            return -1;
        }

        h_args->data = calloc(s->data_size, sizeof(char));
        if (h_args->data  == NULL) {
            free(h_args);
            perror("calloc for data copy");
            return -1;
        }
        strncpy(h_args->data , s->data, s->data_size);

        if (init_addr(&h_args->server_addr, s->server_ip, s->server_port) == -1) {
            free(h_args->data);
            free(h_args);
            return -1;
        }

        h_args->data_size = s->data_size;
        h_args->thread_id = i;
        if(pthread_create(&t_ids[i], NULL, handle, (void *)h_args) != 0) {
            free(h_args->data);
            free(h_args);
            return -1;
        }
        n_threads++;
    }

    return 0;
}

int stop_threads(struct dc_error * err, struct dc_env * env) {
    DC_TRACE(env);
    int ret = 0;

    for (int i = 0; i < n_threads; i++) {
        int result = pthread_cancel(t_ids[i]); // dont break if error, attempt to cancel all
        if (result == -1) {
            perror("canceling thread");
            ret = -1;
        }
    }

    for (int i = 0; i < n_threads; i++) {
        int result = pthread_join(t_ids[i], NULL); // wait for all
        if (result == -1) {
            perror("joining thread");
            ret = -1;
        }
    }

    if (t_ids != NULL) {
        free(t_ids);
        t_ids = NULL;
    }
    n_threads = 0;

    return ret;
}

static int get_processors(int *dst) {
    int processors;

    processors = (int)sysconf(_SC_NPROCESSORS_ONLN);
    if (n_processors == -1) {
        perror("sysconf getting processors");
        return -1;
    }

    *dst = processors;

    return 0;
}
