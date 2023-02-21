#include "handle.h"

#include <log.h>
#include <util.h>

#include <inttypes.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop" // suppress endless loop warning

/**
 * hargs_cleanup_handler
 * <p>
 * frees handle arguments struct on exit.
 * </p>
 * @param args char pointer to data.
 */
static void hargs_cleanup_handler(void *args);

void * handle(void *handle_args) {
    struct handle_args *h_args;
    struct logger log;
    int server_sock;
    uint32_t data_size;
    uint32_t server_resp;
    clock_t  start_time_granular;
    clock_t  end_time_granular;

    h_args = handle_args;
    data_size = h_args->data_size; // done for uint32 cast (htonl)
    pthread_cleanup_push(hargs_cleanup_handler, (void*)h_args) // run hargs_cleanup_handler on thread exit

    while (true) {
        memset(&log, 0, sizeof(struct logger));

        if (TCP_socket(&server_sock) == -1) {
            close_fd(server_sock);
            return NULL;
        }

        log.data_size = data_size;
        log.start_time = time(NULL);
        start_time_granular = clock();

        if (init_connection(server_sock, &h_args->server_addr) == -1) {
            close_fd(server_sock);
            // NOLINTNEXTLINE(concurrency-mt-unsafe) : No threads here
            sleep(1);
        } else {
            uint32_t net_f_size = htonl(data_size);
            if (write_fully(server_sock, &net_f_size, sizeof(net_f_size)) == -1) {
                close_fd(server_sock);
                return NULL;
            }

            if (write_fully(server_sock, h_args->data, data_size) == -1) {
                close_fd(server_sock);
                return NULL;
            }

            server_resp = 0;
            if (read_fully(server_sock, &server_resp, sizeof(server_resp)) == -1) {
                close_fd(server_sock);
                return NULL;
            }
            server_resp = ntohl(server_resp);

            if (close_fd(server_sock) == -1) {
                perror("close server fd");
            }

            log.end_time = time(NULL);
            end_time_granular = clock();
            log.elapsed_time_granular = (double) (end_time_granular - start_time_granular) / CLOCKS_PER_SEC;
            log.server_resp = server_resp;
            log.thread_id = h_args->thread_id;

            if (do_log(&log) == -1) {
                return NULL;
            }
        }
        pthread_testcancel();
    }

    pthread_cleanup_pop(1); // should never reach here, but set to 1 to run data_cleanup_handler anyway
}

static void hargs_cleanup_handler(void *args) {
    struct handle_args * h_args;

    h_args = args;
    free(h_args->data);
    free(h_args);
}

#pragma clang diagnostic pop
