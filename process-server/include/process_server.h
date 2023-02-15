#ifndef PROCESS_SERVER_PROCESS_SERVER_H
#define PROCESS_SERVER_PROCESS_SERVER_H



/**
* setup_process_state
* <p>
        * Set up the state object for the one-to-one server. Add it to the memory manager.
* </p>
* @param mm the memory manager to which the state object will be added
* @return the state object, or NULL and set errno on failure
*/
struct state_object *setup_process_state(struct memory_manager *mm);

/**
 * open_process_server_for_listen
 * <p>
 * Create a socket, bind, and begin listening for connections. Fill necessary fields in the
 * core object.
 * </p>
 * @param co the core object
 * @param ps the state object
 * @param listen_addr the address on which to listen
 * @return 0 on success, -1 and set errno on failure
 */
int open_process_server_for_listen(struct core_object *co, struct parent_struct *ps, struct sockaddr_in *listen_addr);

/**
 * run_process_server
 * <p>
 * Run the process server. Wait for activity on one of the processed sockets; if activity
 * is on the listen socket, accept a new connection. If activity is on any other socket,
 * handle that message.
 * </p>
 * @param co the core object
 * @return 0 on success, -1 and set errno on failure
 */
int run_process_server(struct core_object *co);

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
