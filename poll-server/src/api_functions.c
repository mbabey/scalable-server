#include "../../api_functions.h"

int initialize_server(struct core_object *co)
{
    return RUN_SERVER;
}

int run_server(struct core_object *co)
{
    return CLOSE_SERVER;
}

int close_server(struct core_object *co)
{
    return EXIT_SERVER;
}