#include <pthread.h>
#include <string.h>
#include "log.h"

/**
 * format_write
 * <p>
 * format the logging struct into a single line and append it to the log file.
 * </p>
 * @param l pointer to the logging struct to format
 * @return 0 on success. -1 and set errno on failure.
 */
static int format_write(struct logger * l);

const char * csv_header = "TimeStamp, StartTime, EndTime, ExpectedBytes, ActualBytes, Err\n";

static bool initialized = false;
static FILE * log_file;
pthread_mutex_t log_lock;

int init_logger(struct state * s) {
    if (!initialized) {
        log_file = s->log_file;
        if (pthread_mutex_init(&log_lock, NULL) != 0) {
            perror("init log mutex");
            return -1;
        }
        fwrite(csv_header, strlen(csv_header) + 1, 1, log_file); // TODO: err checking because lazy
        initialized = true;
    }
    return 0;
}

int destroy_logger(void) {
    int ret = 0;

    if (initialized) {
        if (pthread_mutex_destroy(&log_lock) != 0) {
            perror("destroying log mutex");
            ret = -1;
        }

        if (fclose(log_file) == EOF) {
            perror("closing log file");
            ret = 0;
        }
    }
    initialized = false;

    return ret;
}

int log_info(struct logger * l) {
    if (pthread_mutex_lock(&log_lock) != 0) {
        perror("locking log mutex");
        return -1;
    }

    format_write(l);

    if (pthread_mutex_unlock(&log_lock) != 0) {
        perror("unlocking log mutex");
        return -1;
    }

    return 0;
}

static int format_write(struct logger * l) {
    char fmt[1000];

    sprintf(fmt, "%ld, %ld, %ld, %u, %u, %s\n", time(NULL), l->start_time, l->end_time, l->expected_bytes,
            l->actual_bytes, l->err_msg);

    fwrite(fmt, sizeof(fmt), 1, log_file); // TODO: err checking because lazy
    return 0;
}
