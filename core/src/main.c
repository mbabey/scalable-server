#include "../../api_functions.h"
#include "objects.h"
#include "util.h"

#include <dc_application/application.h>
#include <dc_application/options.h>
#include <dc_c/dc_stdlib.h>
#include <dc_c/dc_string.h>
#include <dc_env/env.h>
#include <dc_error/error.h>
#include <mem_manager/manager.h>

#include <dlfcn.h>
#include <getopt.h>
#include <string.h>


#define DEFAULT_LIBRARY "../../one-to-one/cmake-build-debug/libone-to-one.dylib" // TODO: relative path should be changed to absolute.
#define DEFAULT_PORT "5000"
#define DEFAULT_IP "123.123.123.123" // TODO: will need to get the IP address by default

#define LOG_FILE_NAME "logs.csv"
#define LOG_OPEN_MODE "w" // Mode is set to truncate for independent results from each experiment.

#define API_INIT "initialize_server"
#define API_RUN "run_server"
#define API_CLOSE "close_server"

/**
 * api_functions
 * <p>
 * Struct containing pointers to all API functions.
 * </p>
 */
struct api_functions
{
    api initialize_server;
    api run_server;
    api close_server;
};

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
 * setup_core_object
 * <p>
 * Zero the core_object. Setup other objects and attach them to the core_object.
 * Open the log file and attach it to the core object.
 * </p>
 * @param co the core object
 * @return 0 on success. On failure, -1 and set errno.
 */
int setup_core_object(struct core_object *co, const struct dc_env *env, struct dc_error *err, const in_port_t port_num,
                      const char *ip_addr);

/**
 * destroy_core_object
 * <p>
 * Destroy the core object and all of its fields. Does not destroy the state object;
 * the state object must be destroyed by the library destroy_server function.
 * </p>
 * @param co the core object
 */
void destroy_core_object(struct core_object *co);

/**
 * get_api
 * <p>
 * Open a given library and attempt to load API functions into the api_functions struct.
 * </p>
 * @param api_functions struct containing API functions.
 * @param lib_name name of the library.
 * @param env pointer to a dc_env struct.
 * @return The opened library. NULL and set errno on failure.
 */
static void *get_api(struct api_functions *api, const char *lib_name, const struct dc_env *env);

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
 * trace_reporter
 * <p>
 * formatting function for trace reporting.
 * </p>
 * @param env pointer to a dc_env struct
 * @param file_name name of the file the trace occurs in.
 * @param function_name name of the function the trace occurs in.
 * @param line_number the line the trace occurs in.
 */
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
    const char                  *lib_name;
    in_port_t                   port_num;
    const char                  *ip_addr;
    
    struct core_object co;
    int                ret_val;
    void               *lib;
    
    app_settings = (struct application_settings *) settings;
    lib_name     = dc_setting_string_get(env, app_settings->library);
    port_num     = dc_setting_in_port_t_get(env, app_settings->port_num);
    ip_addr      = dc_setting_string_get(env, app_settings->ip_addr);
    
    // create core object
    ret_val = setup_core_object(&co, env, err, port_num, ip_addr);
    if (ret_val == 0)
    {
        struct api_functions api;
        lib = get_api(&api, lib_name, env);
        if (lib == NULL)
        {
            ret_val = -1;
        } else
        {
            // TODO: how do we want to handle return values here?
            ret_val = api.initialize_server(&co);
            // check error
            
            ret_val = api.run_server(&co);
            // check error
            
            ret_val = api.close_server(&co);
            // check error
            
            ret_val = close_lib(lib);
            if (ret_val != 0)
            {
                // NOLINTNEXTLINE(concurrency-mt-unsafe) : No threads here
                (void) fprintf(stderr, "Fatal: could not close lib_name %s: %s\n", lib_name, strerror(errno));
            }
        }
        destroy_core_object(&co);
    }
    return ret_val;
}

int setup_core_object(struct core_object *co, const struct dc_env *env, struct dc_error *err, const in_port_t port_num,
                      const char *ip_addr)
{
    DC_TRACE(env);
    memset(co, 0, sizeof(struct core_object));
    
    co->env = env;
    co->err = err;
    co->mm  = init_mem_manager();
    if (!co->mm)
    {
        // NOLINTNEXTLINE(concurrency-mt-unsafe) : No threads here
        (void) fprintf(stderr, "Fatal: could not initialize memory manager: %s\n", strerror(errno));
        return -1;
    }
    co->log_file = open_file(LOG_FILE_NAME, LOG_OPEN_MODE);
    if (!co->log_file)
    {
        // NOLINTNEXTLINE(concurrency-mt-unsafe) : No threads here
        (void) fprintf(stderr, "Fatal: could not open %s: %s\n", LOG_FILE_NAME, strerror(errno));
        return -1;
    }
    
    if (assemble_listen_addr(&co->listen_addr, port_num, ip_addr) == -1)
    {
        // NOLINTNEXTLINE(concurrency-mt-unsafe) : No threads here
        (void) fprintf(stderr, "Fatal: could not assign server address: %s\n", strerror(errno));
        return -1;
    }
    
    return 0;
}

void destroy_core_object(struct core_object *co)
{
    if (co->log_file)
    {
        (void) fclose(co->log_file);
    }
    free_mem_manager(co->mm);
}

static void *get_api(struct api_functions *api, const char *lib_name, const struct dc_env *env)
{
    DC_TRACE(env);
    void *lib;
    bool get_func_err;
    
    // NOLINTBEGIN(concurrency-mt-unsafe) : No threads here
    lib = open_lib(lib_name, RTLD_LAZY);
    if (lib == NULL)
    {
        (void) fprintf(stderr, "Fatal: could not open API library %s: %s\n", lib_name, strerror(errno));
        return lib;
    }
    
    get_func_err = false;
    api->initialize_server = (int (*)(struct core_object *)) get_func(lib, API_INIT);
    if (api->initialize_server == NULL)
    {
        (void) fprintf(stderr, "Fatal: could not load API function %s: %s\n", API_INIT, strerror(errno));
        get_func_err = true;
    }
    api->run_server = (int (*)(struct core_object *)) get_func(lib, API_RUN);
    if (api->run_server == NULL)
    {
        (void) fprintf(stderr, "Fatal: could not load API function %s: %s\n", API_RUN, strerror(errno));
        get_func_err = true;
    }
    api->close_server = (int (*)(struct core_object *)) get_func(lib, API_CLOSE);
    if (api->close_server == NULL)
    {
        (void) fprintf(stderr, "Fatal: could not load API function %s: %s\n", API_CLOSE, strerror(errno));
        get_func_err = true;
    }
    // NOLINTEND(concurrency-mt-unsafe)
    
    if (get_func_err)
    {
        close_lib(lib);
        return NULL;
    }
    
    return lib;
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
    (void) fprintf(stdout, "TRACE: %s : %s : @ %zu\n", file_name, function_name, line_number);
}
