#ifndef SCALABLE_SERVER_POLL_SERVER_H
#define SCALABLE_SERVER_POLL_SERVER_H

/**
 * setup_poll_state
 * <p>
 * Set up the state object for the one-to-one server. Add it to the memory manager.
 * </p>
 * @param mm the memory manager to which the state object will be added
 * @return the state object, or NULL and set errno on failure
 */
struct state_object *setup_poll_state(struct memory_manager *mm);

/**
 * open_poll_server_for_listen
 * <p>
 * Create a socket, bind, and begin listening for connections. Fill necessary fields in the
 * core object.
 * </p>
 * @param co the core object
 * @param so the state object
 * @param listen_addr the address on which to listen
 * @return 0 on success, -1 and set errno on failure
 */
int open_poll_server_for_listen(struct core_object *co, struct state_object *so, struct sockaddr_in *listen_addr);

/**
 * run_poll_server
 * <p>
 * Run the poll server. Wait for activity on one of the polled sockets; if activity
 * is on the listen socket, accept a new connection. If activity is on any other socket,
 * handle that message.
 * </p>
 * @param co the core object
 * @return 0 on success, -1 and set errno on failure
 */
int run_poll_server(struct core_object *co);

/**
 * destroy_poll_state
 * <p>
 * Close all connections and all open sockets.
 * </p>
 * @param co the core object
 * @param so the state object
 */
void destroy_poll_state(struct core_object *co, struct state_object *so);

#endif //SCALABLE_SERVER_POLL_SERVER_H
