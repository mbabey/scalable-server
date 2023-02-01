#include "util.h"

#include <dlfcn.h>
#include <netinet/in.h>
#include <arpa/inet.h>

FILE *open_file(const char * file_name, const char * mode)
{
    FILE *file;
    
    file = fopen(file_name, mode); // fixme: This is returning NULL and setting Read-only file system error
    // If an error occurs will return null.
    
    return file;
}

struct sockaddr_in *assemble_listen_addr(struct memory_manager *mm, const in_port_t port_num, const char *ip_addr)
{
    struct sockaddr_in *listen_addr;
    
    listen_addr = (struct sockaddr_in *) Mmm_calloc(1, sizeof(struct sockaddr_in), mm);
    
    //TODO(max): error checking
    listen_addr->sin_port = htonl(port_num);
    listen_addr->sin_family = AF_INET;
    inet_pton(AF_INET, ip_addr, &listen_addr->sin_addr.s_addr);
    
    return listen_addr;
}

void *open_lib(const char *lib_name, int mode)
{
    void *lib;

    lib = dlopen(lib_name, mode);
    // If an error occurs will return null.

    return lib;
}

int close_lib(void * lib) {
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
