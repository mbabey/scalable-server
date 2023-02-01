#include "util.h"

#include <arpa/inet.h>
#include <dlfcn.h>
#include <string.h>

FILE *open_file(const char *file_name, const char *mode)
{
    FILE *file;
    
    file = fopen(file_name, mode); // fixme: This is returning NULL and setting Read-only file system error
    // If an error occurs will return null.
    
    return file;
}

int assemble_listen_addr(struct sockaddr_in *listen_addr, const in_port_t port_num, const char *ip_addr)
{
    int ret_val;
    
    memset(listen_addr, 0, sizeof(struct sockaddr_in));
    
    listen_addr->sin_port   = htonl(port_num);
    listen_addr->sin_family = AF_INET;
    switch (inet_pton(AF_INET, ip_addr, &listen_addr->sin_addr.s_addr))
    {
        case 1: // Valid
        {
            ret_val = 0;
            break;
        }
        case 0: // Not a valid IP address
        {
            fprintf(stderr, "%s is not a valid IP address\n", ip_addr);
            ret_val = -1;
            break;
        }
        default:
        {
            ret_val = -1;
            break;
        }
    }
    
    return ret_val;
}

void *open_lib(const char *lib_name, int mode)
{
    void *lib;
    
    lib = dlopen(lib_name, mode);
    // If an error occurs will return null.
    
    return lib;
}

int close_lib(void *lib)
{
    return dlclose(lib);
    // If an error occurs will return null.
}

void *get_func(void *lib, const char *func_name)
{
    void *func;
    
    func = dlsym(lib, func_name);
    // If an error occurs will return null.
    
    return func;
}
