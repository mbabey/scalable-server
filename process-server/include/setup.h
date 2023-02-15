#ifndef PROCESS_SERVER_SETUP_H
#define PROCESS_SERVER_SETUP_H

#include "objects.h"

/**
 * setup_process_state
 * <p>
 * Set up the state object for the process server. Add it to the memory manager.
 * </p>
 * @param mm the memory manager to which the state object will be added
 * @return the state object, or NULL and set errno on failure
 */
struct state_object *setup_process_state(struct memory_manager *mm);

/**
 * open_domain_socket_setup_semaphores
 * <p>
 * Open the domain socket and set up the semaphores for controlling access to the child-parent pipe and the log
 * file.
 * </p>
 * @return 0 on success, -1 and set errno on failure
 */
int open_domain_socket_setup_semaphores(struct core_object *co, struct state_object *so);

/**
 * p_open_process_server_for_listen
 * <p>
 * Create a socket, bind, and begin listening for connections. Fill necessary fields in the
 * core object.
 * </p>
 * @param co the core object
 * @param parent the parent object
 * @param listen_addr the address on which to listen
 * @return 0 on success, -1 and set errno on failure
 */
static int p_open_process_server_for_listen(struct core_object *co, struct parent_struct *parent, struct sockaddr_in *listen_addr);

#endif //PROCESS_SERVER_SETUP_H
