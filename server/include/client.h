#ifndef CLIENT 
#define CLIENT

#include <queue>
#include <string>
#include <unistd.h>
#include <semaphore.h>

#include "../include/connection.h"

/*
 * The different types of events that might need to be broadcast to all client
 */
typedef enum {
    event_file_modified = 1,
    event_file_deleted = 2,
} file_event_type_t;

/*
 * The file event that might need to be broadcast to all clients
 */
typedef struct {
    file_event_type_t type;
    std::string filename;
} file_event_t;

/*
 * The client from the server's perspective
 */
typedef struct {
    /*
     * The client's username.
     */
    std::string username;

    /*
     * The server's TCP connection with the clients. Onwed pointer.
     */
    connection_t *connection;

    /*
     * Whether the client is considered active by the application.
     */
    bool active;

    /*
     * A queue of all the untreated events
     */
    std::queue<file_event_t> pending_events;

    /*
     * A mutex that prevents mutual access of the client
     */
    sem_t mutex;
} client_t;

/*
 * Takes connection_t pointer as an owned pointer.
 *
 * Returns a nullptr and closes the TCP connection, if there are too many connections to the same username.
 */
client_t *client_new(std::string username, connection_t *connection);

/*
 * Frees the memory from the client connection. The TCP connection is garanteed to be closed.
 */
void client_free(client_t *client);

/*
 * Broadcasts the `file modified` event to all clients currently connected
 */
void client_broadcast_file_modified(std::string username, std::string filename);

/*
 * Broadcasts the `file deleted` event to all clients currently connected
 */
void client_broadcast_file_deleted(std::string username, std::string filename);

/*
 * Attempts to get the next event in the queue
 */
bool client_get_event(client_t *client, file_event_t *out_event);

#endif // CLIENT
