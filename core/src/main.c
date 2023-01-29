#include "../include/objects.h"

#include <dc_env/env.h>
#include <dc_error/error.h>
#include <dc_application/application.h>
#include <dc_application/options.h>
#include <dc_c/dc_stdlib.h>
#include <dc_c/dc_string.h>
#include <dc_c/dc_stdio.h>
#include <mem_manager/manager.h>

#include <string.h>
#include <getopt.h>

#define LOG_FILE_NAME "logs.csv"
#define LOG_OPEN_MODE "w" // Mode is set to truncate for independent results from each experiment.

// TODO: Need documentation
struct application_settings
{
    struct dc_opt_settings   opts;
    struct dc_setting_string *library;
    // storing a struct is not possible, only use as app settings for now
};

// TODO: Need documentation.
static struct dc_application_settings *create_settings(const struct dc_env *env, struct dc_error *err);

/**
 * setup_core_object
 * <p>
 * Zero the core_object. Setup other objects and attach them to the core_object.
 * Open the log file and attach it to the core object.
 * </p>
 * @param co the core object
 * @return 0 on success. On failure, -1 and set errno
 */
int setup_core_object(struct core_object *co, const struct dc_env *env, struct dc_error *err);

/**
 * open_log_file
 * <p>
 * Open the file to log the results of running the program.
 * </p>
 * @return The log file. NULL and set errno on failure.
 */
FILE *open_log_file(void);

/**
 * destroy_core_object
 * <p>
 * Destroy the core object and all of its fields.
 * </p>
 * @param co the core object
 * @return 0 on success. -1 and set errno on failure
 */
int destroy_core_object(struct core_object *co);

// TODO: Need documentation.
static int run(const struct dc_env *env, struct dc_error *err, struct dc_application_settings *settings);

// TODO: Need documentation.
static int destroy_settings(const struct dc_env *env, struct dc_error *err, struct dc_application_settings **psettings);

// TODO: Need documentation.
static void
trace_reporter(const struct dc_env *env, const char *file_name, const char *function_name, size_t line_number);

int main(int argc, char *argv[])
{
    int                        ret_val;
    dc_env_tracer              tracer;
    struct dc_env              *env;
    struct dc_error            *err;
    struct dc_application_info *info;
    
    tracer = NULL;
    //tracer = trace_reporter;
    err    = dc_error_create(false);
    env    = dc_env_create(err, false, tracer);
    info   = dc_application_info_create(env, err, "scalable_server");
    
    ret_val = dc_application_run(env, err, info, create_settings, destroy_settings, run, dc_default_create_lifecycle,
                                 dc_default_destroy_lifecycle, NULL, argc, argv);
    
    dc_application_info_destroy(env, &info);
    dc_error_reset(err);
    
    return ret_val;
}

static struct dc_application_settings *create_settings(const struct dc_env *env, struct dc_error *err)
{
    struct application_settings *settings;
    
    DC_TRACE(env);
    settings = dc_malloc(env, err, sizeof(struct application_settings));
    
    if (settings == NULL)
    {
        return NULL;
    }
    
    settings->opts.parent.config_path = dc_setting_path_create(env, err);
    settings->library                 = dc_setting_string_create(env, err);
    
    struct options opts[] = {
            {(struct dc_setting *) settings->opts.parent.config_path,
                    dc_options_set_path,
                    "config",
                    required_argument,
                    'c',
                    "CONFIG",
                    dc_string_from_string,
                    NULL,
                    dc_string_from_config,
                    NULL},
            {(struct dc_setting *) settings->library,
                    dc_options_set_string,
                    "library",
                    required_argument,
                    'l',
                    "LIBRARY",
                    dc_string_from_string,
                    "library",
                    dc_string_from_config,
                    "one-to-one"}, // one-to-one is the default library.
    };
    
    settings->opts.opts_count = (sizeof(opts) / sizeof(struct options)) + 1;
    settings->opts.opts_size  = sizeof(struct options);
    settings->opts.opts       = dc_calloc(env, err, settings->opts.opts_count, settings->opts.opts_size);
    dc_memcpy(env, settings->opts.opts, opts, sizeof(opts));
    settings->opts.flags      = "m:";
    settings->opts.env_prefix = "SCALABLE_SERVER_";
    
    return (struct dc_application_settings *) settings;
}

static int run(const struct dc_env *env, struct dc_error *err, struct dc_application_settings *settings)
{
    DC_TRACE(env);
    struct application_settings *app_settings;
    const char                  *library;
    
    int                ret_val;
    struct core_object co;
    
    app_settings = (struct application_settings *) settings;
    library      = dc_setting_string_get(env, app_settings->library);
    
    // create core object
    ret_val = setup_core_object(&co, env, err);
    if (!ret_val)
    {
        // Open library with void *lib = dlopen(libname, RTLD_LAZY);
        
        // link api_functions to function pointers with dlsym(lib, "function_name");
        
        // run functions
        // initialize_server(co);
        // run_server(co)
        // close_server(co);
        
        // Close library with dlclose(lib);
    }
    
    return ret_val;
}

int setup_core_object(struct core_object *co, const struct dc_env *env, struct dc_error *err)
{
    memset(co, 0, sizeof(struct core_object));
    
    co->env = env;
    co->err = err;
    co->mm  = init_mem_manager();
    if (!co->mm)
    {
        (void) fprintf(stderr, "Fatal: could not initialize memory manager: %s\n", strerror(errno));
        return -1;
    }
    
    co->log_file = open_log_file();
    if (!co->log_file)
    {
        (void) fprintf(stderr, "Fatal: could not open %s: %s\n", LOG_FILE_NAME, strerror(errno));
        return -1;
    }
    
    return 0;
}

FILE *open_log_file(void)
{
    FILE       *log_file;
    const char *file_name;
    
    file_name = LOG_FILE_NAME;
    
    log_file = fopen(file_name, LOG_OPEN_MODE);
    // If an error occurs will return null.
    
    return log_file;
}

int destroy_core_object(struct core_object *co)
{
    if (co->log_file)
    {
        fclose(co->log_file);
    }
    
    return 0;
}

static int destroy_settings(const struct dc_env *env, struct dc_error *err, struct dc_application_settings **psettings)
{
    struct application_settings *app_settings;
    
    DC_TRACE(env);
    app_settings = (struct application_settings *) *psettings;
    dc_setting_string_destroy(env, &app_settings->library);
    dc_free(env, app_settings->opts.opts);
    dc_free(env, *psettings);
    
    if (dc_env_is_zero_free(env))
    {
        *psettings = NULL;
    }
    
    return 0;
}

static void
trace_reporter(const struct dc_env *env, const char *file_name, const char *function_name, size_t line_number)
{
    fprintf(stdout, "TRACE: %s : %s : @ %zu\n", file_name, function_name, line_number);
}
