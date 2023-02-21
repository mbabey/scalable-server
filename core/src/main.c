#include "util.h"

#include <dc_application/options.h>
#include <dc_c/dc_stdlib.h>
#include <dc_c/dc_string.h>
#include <mem_manager/manager.h>

#include <getopt.h>
#include <string.h>
#include <dlfcn.h>

#define LOG_FILE_NAME "log.csv"
#define LOG_OPEN_MODE "w" // Mode is set to truncate for independent results from each experiment.

// TODO: relative path should be changed to absolute.
#ifdef __linux__
#define DEFAULT_LIBRARY "../../one-to-one/cmake-build-debug/libone-to-one.so"
#else
#define DEFAULT_LIBRARY "../../one-to-one/cmake-build-debug/libone-to-one.dylib"
#endif

#define API_INIT "initialize_server"
#define API_RUN "run_server"
#define API_CLOSE "close_server"


/**
 * application_settings
 * <p>
 * Struct of settings that control the application.
 * </p>
 */
struct application_settings
{
    struct dc_opt_settings      opts;
    struct dc_setting_string    *library;
    struct dc_setting_in_port_t *port_num;
    struct dc_setting_string    *ip_addr;
    // storing a struct is not possible, only use as app settings for now
};

/**
 * create_settings
 * <p>
 * Defines application settings and allocates memory for an application settings struct.
 * </p>
 * @param env pointer to a dc_env struct.
 * @param err pointer to a dc_error struct.
 * @return a pointer to the settings object.
 */
static struct dc_application_settings *create_settings(const struct dc_env *env, struct dc_error *err);

/**
 * run
 * <p>
 * Runs the application.
 * </p>
 * @param env pointer to a dc_env struct.
 * @param err pointer to a dc_error struct.
 * @param settings pointer to the application settings struct.
 * @return 0. -1 if an error occurred.
 */
static int run(const struct dc_env *env, struct dc_error *err, struct dc_application_settings *settings);

/**
 * destroy_settings
 * <p>
 * Frees memory allocated for the application settings struct.
 * </p>
 * @param env pointer to a dc_env struct.
 * @param err pointer to a dc_error struct.
 * @param psettings double pointer to the application settings struct to free.
 * @return 0.
 */
static int destroy_settings(const struct dc_env *env, struct dc_error *err, struct dc_application_settings **psettings);

/**
 * run_core
 * <p>
 * Open the library, switch on the states returned by api functions.
 * </p>
 * @param co the core object
 * @param lib_name the name of the library to open
 * @return 0 on a success exit, -1 on a failure exit
 */
static int run_core(struct core_object *co, const char *lib_name);

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
    settings->port_num                = dc_setting_in_port_t_create(env, err);
    settings->ip_addr                 = dc_setting_string_create(env, err);
    
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
                    DEFAULT_LIBRARY},
            {(struct dc_setting *) settings->port_num,
                    dc_options_set_in_port_t,
                    "port",
                    required_argument,
                    'p',
                    "PORT",
                    dc_in_port_t_from_string,
                    "port",
                    dc_in_port_t_from_config,
                    DEFAULT_PORT},
            {(struct dc_setting *) settings->ip_addr,
                    dc_options_set_string,
                    "ip-addr",
                    required_argument,
                    'i',
                    "IP_ADDR",
                    dc_string_from_string,
                    "ip-addr",
                    dc_string_from_config,
                    DEFAULT_IP},
    };
    
    settings->opts.opts_count = (sizeof(opts) / sizeof(struct options)) + 1;
    settings->opts.opts_size  = sizeof(struct options);
    settings->opts.opts       = dc_calloc(env, err, settings->opts.opts_count, settings->opts.opts_size);
    dc_memcpy(env, settings->opts.opts, opts, sizeof(opts));
    settings->opts.flags      = "l:p:i:";
    settings->opts.env_prefix = "SCALABLE_SERVER_";
    
    return (struct dc_application_settings *) settings;
}

static int run(const struct dc_env *env, struct dc_error *err, struct dc_application_settings *settings)
{
    DC_TRACE(env);
    struct application_settings *app_settings;
    struct core_object          co;
    const char                  *lib_name;
    in_port_t                   port_num;
    const char                  *ip_addr;
    
    int ret_val;
    
    app_settings = (struct application_settings *) settings;
    lib_name     = dc_setting_string_get(env, app_settings->library);
    port_num     = dc_setting_in_port_t_get(env, app_settings->port_num);
    ip_addr      = dc_setting_string_get(env, app_settings->ip_addr);
    
    // create core object
    ret_val = setup_core_object(&co, env, err, port_num, ip_addr);
    if (ret_val == -1)
    {
        return EXIT_FAILURE;
    }
    
    ret_val = run_core(&co, lib_name);
    
    destroy_core_object(&co);
    return ret_val;
}

static int run_core(struct core_object *co, const char *lib_name)
{
    struct api_functions api;
    void                 *lib;
    int                  next_state;
    int                  exit_status;
    int                  run;
    
    lib         = get_api(&api, lib_name, co->env);
    run         = (lib) ? 1 : 0; // Run if lib not null.
    exit_status = (lib) ? EXIT_SUCCESS : EXIT_FAILURE; // Set initial exit code.
    next_state  = INITIALIZE_SERVER;
    while (run)
    {
        switch (next_state)
        {
            case INITIALIZE_SERVER:
            {
                next_state = api.initialize_server(co);
                break;
            }
            case RUN_SERVER:
            {
                next_state = api.run_server(co);
                break;
            }
            case CLOSE_SERVER:
            {
                next_state = api.close_server(co);
                break;
            }
            case ERROR:
            {
                // NOLINTNEXTLINE(concurrency-mt-unsafe) : No threads here
                (void) fprintf(stderr, "Fatal: error during server runtime: %s\n", strerror(errno));
                next_state  = api.close_server(co);
                exit_status = EXIT_FAILURE;
                break;
            }
            case EXIT:
            {
                run = 0;
                if (!exit_status) // If exit_status is not already EXIT_FAILURE
                {
                    exit_status = EXIT_SUCCESS;
                }
                break;
            }
            default: // Should not get here.
            {
                run = 0;
            }
        }
    }
    
    if (lib)
    {
        close_lib(lib, lib_name); // TODO(): How to handle error here?
    }
    
    return exit_status;
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
