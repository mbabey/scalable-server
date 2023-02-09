#include "handle.h"

#include <log.h>
#include <util.h>

#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop" // suppress endless loop warning

static void cleanup_handler(void *args);

void * handle(void *handle_args) {
    int result;
    int server_sock;
    struct logger log;
    struct handle_args *h_args;

    h_args = handle_args;
    pthread_cleanup_push(cleanup_handler, (void*)h_args->data) // run cleanup_handler on thread exit

    while (true) {
        if (TCP_socket(&server_sock) == -1) {
            free(h_args->data);
            return NULL;
        }
        if (init_connection(server_sock, &h_args->server_addr) == -1) {
            log.error = true;
            log.err_msg = "could not connect to server";
        } else {
            // log start time

            // write

            // read

            // log end time
        }


        pthread_testcancel();
    }

    pthread_cleanup_pop(1) // should never reach here, but set to 1 to run cleanup_handler anyway
}

static void cleanup_handler(void *args) {
        char * data;

        data = args;
        free(data);
}

#pragma clang diagnostic pop
