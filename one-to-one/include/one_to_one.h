#ifndef ONE_TO_ONE_ONE_TO_ONE_H
#define ONE_TO_ONE_ONE_TO_ONE_H

#include <mem_manager/manager.h>
#include <netinet/in.h>
#include <signal.h>

/**
 * The number of connections that can be queued on the listening socket.
 */
#define CONNECTION_QUEUE 10

/**
 * Enumerator to handle client connection
 */
enum handle_client_result{
    CLIENT_RESULT_ERROR,
    CLIENT_RESULT_TERMINATION,
    CLIENT_RESULT_SUCCESS,
};

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
 */
void destroy_state(struct state_object *so);

/**
 * run_one_to_one
 * <p>
 * Run the server
 * </p>
 * @param fd as int
 * @return 0 on success, -1 and set errno on failure
 */
int handle_client (struct core_object *co);

/**
 * accept_conn
 * <p>
 * Accept connection
 * </p>
 * @param listen_fd as int
 * @return int on accepted socket, -1 and set errno on failure
 */
int accept_conn(int listen_fd, int* fd_out);

/**
 * signal_handler
 * <p>
 * Handles signal
 * <p>
 * @param sig_num as int
 */
void handle_sigint(int sig_num);

/**
 * set_signal_hadnler
 * @param sa as sigaction
 * @param signal_handler as function pointer accepting parameter int and returning void
 */
int set_signal_handler(struct sigaction *sa, void (*signal_handler)(int));
#endif //ONE_TO_ONE_ONE_TO_ONE_H
