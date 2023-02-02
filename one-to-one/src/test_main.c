#include "../../core/include/util.h"

#include <dc_error/error.h>
#include <stdlib.h>

#define PORT 5000
#define IP_ADDR "123.123.123.123"

int main(void)
{
    struct core_object co;
    struct dc_env *env;
    struct dc_error *err;
    
    err = dc_error_create(true);
    env = dc_env_create(err, false, );
    
    setup_core_object(&co, )
    
    return EXIT_SUCCESS;
}

