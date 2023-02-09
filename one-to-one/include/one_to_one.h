#ifndef ONE_TO_ONE_ONE_TO_ONE_H
#define ONE_TO_ONE_ONE_TO_ONE_H

#include <mem_manager/manager.h>
#include <netinet/in.h>

/**
 * The number of connections that can be queued on the listening socket.
 */
#define CONNECTION_QUEUE 10

/**
 * setup_state
 * <p>
 * Set up the state object for the one-to-one server. Add it to the memory manager.
 * </p>
 * @param mm the memory manager to which the state object will be added
 * @return the state object, or NULL and set errno on failure
 */
struct state_object *setup_state(struct memory_manager *mm);

/**
 * open_server_for_listen
 * <p>
 * Create a socket, bind, and begin listening for connections. Fill necessary fields in the
 * core object.
 * </p>
 * @param so the state object
 * @param listen_addr the address on which to listen
 * @return 0 on success, -1 and set errno on failure
 */
int open_server_for_listen(struct state_object *so, struct sockaddr_in *listen_addr);

/**
 * destroy_state
 * <p>
 * Close all connections and all open sockets.
 * </p>
 * @param so the state object
 * @return 0 on success, -1 and set errno on failure
 */
int destroy_state(struct state_object *so);

/**
 * run_one_to_one
 * <p>
 * Run the server
 * </p>
 * @param so the state object
 * @return 0 on success, -1 and set errno on failure
 */
int run_one_to_one(struct state_object *so);
#endif //ONE_TO_ONE_ONE_TO_ONE_H
