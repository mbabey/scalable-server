#include "log.h"

#include <util.h>

#include <inttypes.h>
#include <pthread.h>
#include <string.h>

#define LOG_FILE_NAME "log.csv"
#define LOG_OPEN_MODE "w" // Mode is set to truncate for independent results from each experiment.

/**
 * log
 * <p>
 * Log the information from one received message into the log file in Comma Separated Value file format.
 * </p>
 * @param l pointer to the logger struct.
 */
static void log(struct logger * l);

const char * csv_header = "TimeStamp, ThreadID, DataSize, ServerResponse, StartTime, EndTime, ElapsedTime\n";

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

int do_log(struct logger * l) {
    if (pthread_mutex_lock(&log_lock) != 0) {
        perror("locking log mutex");
        return -1;
    }

    log(l);

    if (pthread_mutex_unlock(&log_lock) != 0) {
        perror("unlocking log mutex");
        return -1;
    }

    return 0;
}

static void log(struct logger * l) {
    time_t    time_stamp;
    char      *time_stamp_str;
    char      *start_time_str;
    char      *end_time_str;

    // NOLINTBEGIN(concurrency-mt-unsafe): Mutex being used
    time_stamp = time(NULL);
    time_stamp_str = ctime(&time_stamp);
    *(time_stamp_str+ strlen(time_stamp_str) - 1) = '\0';
    start_time_str = (l->start_time == 0) ? "NULL\0" : ctime(&l->start_time);
    *(start_time_str + strlen(start_time_str) - 1) = '\0';
    end_time_str = (l->end_time == 0) ? "NULL\0" : ctime(&l->end_time);
    *(end_time_str + strlen(end_time_str) - 1) = '\0';
    // NOLINTEND(concurrency-mt-unsafe)

    (void) fprintf(log_file, "%s, %d, %"PRIu32", %"PRIu32", %s, %s, %lf\n", time_stamp_str, l->thread_id, l->data_size, l->server_resp, start_time_str, end_time_str, l->elapsed_time_granular);
}
