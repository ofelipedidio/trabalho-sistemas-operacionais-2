#include "../include/coms.h"
#include <semaphore.h>

typedef struct {
    // Static
    server_t current_server;

    // Dynamic
    sem_t coms_server_mutex;
    coms_server_t coms_server;
} program_state_t;

/*
 * Initializes the state of the program
 */
void state_init();

/*
 * Acquires the coms_server mutex and returns it
 */
coms_server_t *acquire_coms_server();

/*
 * Releases the coms_server mutex
 */
void release_coms_server();

/*
 * Sets the value of coms_server
 * Assumes you have the com_server mutex
 */
void set_coms_server(coms_server_t *coms_server);

/*
 * Acquires the lock and returns the current server
 */
server_t *acquire_current_server();

/*
 * Releases the the current server's lock
 */
server_t *release_current_server();
