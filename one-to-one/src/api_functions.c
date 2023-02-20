#include "../../api_functions.h"
#include "../include/objects.h"
#include "../include/one_to_one.h"

#include <stdio.h>
#include <unistd.h>
#include <signal.h>

int initialize_server(struct core_object *co)
{
    printf("INIT ONE-TO-ONE SERVER!!\n");
    
    co->so = setup_state(co->mm);
    if (!co->so)
    {
        return ERROR;
    }
    
    if (open_server_for_listen(co->so, &co->listen_addr) == -1)
    {
        return ERROR;
    }

    struct sigaction sa;
    if (set_signal_handler(&sa, handle_sigint) == -1) {
        return ERROR;
    }


    return RUN_SERVER;
}

int run_server(struct core_object *co) {
    printf("RUN ONE-TO-ONE SERVER\n");
    int handle_result;
    do {
        close(co->so->client_fd);
        int accept_result = accept_conn(co->so->listen_fd, &co->so->client_fd);
        if(accept_result == CLIENT_RESULT_TERMINATION){
            return CLOSE_SERVER;
        }
        if (accept_result == CLIENT_RESULT_ERROR) {
            return ERROR;
        }
    }
    while ((handle_result = handle_client(co)) == CLIENT_RESULT_SUCCESS);
    if (handle_result == CLIENT_RESULT_ERROR){
        return ERROR;
    }
    return CLOSE_SERVER;
}

int close_server(struct core_object *co)
{
    printf("CLOSE ONE-TO-ONE SERVER\n");
    
    destroy_state(co->so);
    
    return EXIT;
}
