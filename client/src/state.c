#include "../include/state.h"

#include <log.h>
#include <thread.h>
#include <util.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define DATA_OPEN_MODE "r"

/**
 * load_data
 * <p>
 * allocates space for and reads a file.
 * </p>
 * @param dst where to allocate and store data.
 * @param file_name name of the file to read.
 * @param mode mode to open the file in.
 * @return 0 on success. -1 on failure and set errno.
 */
static int load_data(char **dst, const char *file_name, const char *mode, struct dc_env * env);

static int validate_params(struct init_state_params * params, struct dc_env * env);

int init_state(struct init_state_params * params, struct state * s, struct dc_error * err, struct dc_env * env) {
    DC_TRACE(env);

    if (validate_params(params, env) == -1) return -1;

    memset(s, 0, sizeof(struct state));

    s->server_ip = params->server_ip;
    s->controller_ip = params->controller_ip;

    if (parse_port(&s->server_port, params->server_port, 10) == -1) return -1;
    if (parse_port(&s->controller_port, params->controller_port, 10) == -1) return -1;

    if (init_addr(&s->server_addr, s->server_ip, s->server_port) == -1) return -1;
    if (init_addr(&s->controller_addr, s->controller_ip, s->controller_port) == -1) return -1;

    if (TCP_socket(&s->controller_fd) == -1) return -1;

    if (init_connection(s->controller_fd, &s->controller_addr) == -1) return -1;

    if (set_sock_blocking(s->controller_fd, false) == -1) return -1; // needed for poll

    if (load_data(&s->data, params->data_file_name, DATA_OPEN_MODE, env) == -1) return -1;

    if (init_logger() == -1) return -1;

    return 0;
}

static int validate_params(struct init_state_params * params, struct dc_env * env) {
    DC_TRACE(env);

    if (params->server_ip == NULL) {
        (void) fprintf(stderr, "server IP required, pass with -s");
        return -1;
    }
    if (params->controller_ip == NULL) {
        (void) fprintf(stderr, "controller IP required, pass with -c");
        return -1;
    }
    if (params->data_file_name == NULL) {
        (void) fprintf(stderr, "data file required, pass with -d");
        return -1;
    }
    return 0;
}

static int load_data(char **dst, const char *file_name, const char *mode, struct dc_env * env) {
    DC_TRACE(env);
    FILE * data_file;
    struct stat data_info;
    size_t nread;
    int result;

    result = stat(file_name, &data_info);
    if (result == -1) {
        perror("stat on data file");
        return -1;
    }

    *dst = malloc(data_info.st_size);
    if (*dst == NULL) {
        perror("malloc for data");
        return -1;
    }

    if (open_file(&data_file, file_name, mode) == -1) return -1;

    nread = fread(*dst, sizeof(char), data_info.st_size, data_file);
    if (nread < (size_t)data_info.st_size) {
        if (feof(data_file))
            (void) fprintf(stderr, "reading data file: unexpected end of file\n");
        else if (ferror(data_file)) {
            perror("reading data file");
        }
        result = -1;
    }

    // close file regardless of result
    if (fclose(data_file) == -1) {
        perror("closing data file");
        result = -1;
    }

    return result;
}

int destroy_state(struct state * s, struct dc_error * err, struct dc_env * env) {
    DC_TRACE(env);
    int result;
    int ret;

    ret = stop_threads(err, env);

    if (s->controller_fd) {
        result = close(s->controller_fd);
        if (result == 1) {
            perror("closing controller connection");
            ret = 1;
        }
    }

    if (s->data) {
        free(s->data);
    }

    if (destroy_logger() == -1) {
        ret = -1;
    }

    return ret;
}
