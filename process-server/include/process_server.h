#ifndef PROCESS_SERVER_PROCESS_SERVER_H
#define PROCESS_SERVER_PROCESS_SERVER_H

#include "objects.h"

/**
 * setup_process_server
 * <p>
 * Perform all setup necessary for the process server. Setup the state object, open the domain socket,
 * set up the semaphores, then fork the process. Assign all process ids in the state object and open the server
 * socket for listening in the parent. Allocate memory for the parent object in the parent. Allocate memory for
 * the child object in the children.
 * </p>
 * @param co the core object
 * @return 0 on success, -1 and set errno on failure
 */
int setup_process_server(struct core_object *co, struct state_object *so);

/**
 * run_process_server
 * <p>
 * Run the process server. Wait for activity on one of the processed sockets; if activity
 * is on the listen socket, accept a new connection. If activity is on any other socket,
 * handle that message by sending to one of the free child labourer processes.
 * </p>
 * @param co the core object
 * @return 0 on success, -1 and set errno on failure
 */
int run_process_server(struct core_object *co, struct state_object *so);

/**
 * destroy_process_state
 * <p>
 * Close all connections and all open sockets.
 * </p>
 * @param co the core object
 * @param so the state object
 */
void destroy_process_state(struct core_object *co, struct state_object *so);

#endif //PROCESS_SERVER_PROCESS_SERVER_H
