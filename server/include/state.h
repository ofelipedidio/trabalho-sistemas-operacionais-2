#include "../include/coms.h"
#include <semaphore.h>

// TODO - Didio: implement read-write lock

typedef struct {
    server_t current_server;
    server_t primary_server;

    sem_t metadata_mutex;
    metadata_t metadata;

    sem_t coms_server_mutex;
    coms_server_t coms_server;

    bool should_stop;

    sem_t logging_mutex;
} program_state_t;

/****************\
* Initialization *
\****************/
void state_init(uint32_t ip, uint16_t port, server_type_t type);

/*************\
* Should stop *
\*************/
bool should_stop();

/*************\
* Coms server *
\*************/
coms_server_t *acquire_coms_server();

void release_coms_server();

void set_coms_server(coms_server_t *coms_server);

/****************\
* Current server *
\****************/
server_t *get_current_server();

/*****************\
* Primary server *
\*****************/
server_t *get_primary_server();

/**********\
* Metadata *
\**********/
metadata_t *acquire_metadata();

void release_metadata();

void acquire_logging_mutex();

void release_logging_mutex();

#define LOG_SYNC(x) acquire_logging_mutex(); x; release_logging_mutex()
