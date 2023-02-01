#ifndef ONE_TO_ONE_SETUP_H
#define ONE_TO_ONE_SETUP_H

#include <mem_manager/manager.h>

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
 * @param co the core object
 * @return 0 on success, -1 and set errno on failure
 */
int open_server_for_listen(struct core_object *co);

#endif //ONE_TO_ONE_SETUP_H
