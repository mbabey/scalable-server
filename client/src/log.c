#include "log.h"

#include <util.h>

#include <pthread.h>
#include <string.h>

#define LOG_FILE_NAME "log.csv"
#define LOG_OPEN_MODE "w" // Mode is set to truncate for independent results from each experiment.
#define LOG_LINE_BUFFER 1000 // Used to hold a formatted log line.

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

int init_logger(void) {
    int result = 0;

    if (!initialized) {
        if (open_file(&log_file, LOG_FILE_NAME, LOG_OPEN_MODE) == -1) {
            return -1;
        }

        if (pthread_mutex_init(&log_lock, NULL) != 0) {
            perror("init log mutex");
            return -1;
        }

        (void) fwrite(csv_header, strlen(csv_header) + 1, 1, log_file);
        if (feof(log_file)) {
            (void) fprintf(stderr, "reading data file: unexpected end of file\n");
            result = -1;
        } else if (ferror(log_file)) {
            perror("reading data file");
            result = -1;
        }
    }

    if (result == -1) {
        if (pthread_mutex_destroy(&log_lock) != 0) {
            perror("destroying log mutex");
        }
    } else {
        initialized = true;
    }

    return result;
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
    char fmt[LOG_LINE_BUFFER];

    if (sprintf(fmt, "%ld, %ld, %ld, %u, %u, %s\n", time(NULL), l->start_time, l->end_time, l->expected_bytes,
            l->actual_bytes, l->err_msg) < 0) {
        perror("formatting log line");
        return -1;
    }

    (void) fwrite(fmt, sizeof(fmt), 1, log_file);
    if (feof(log_file)) {
        (void) fprintf(stderr, "reading data file: unexpected end of file\n");
        return -1;
    } else if (ferror(log_file)) {
        perror("reading data file");
        return -1;
    }
    return 0;
}
