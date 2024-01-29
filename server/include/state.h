#include "../include/coms.h"
#include <semaphore.h>
#include <vector>

// TODO - Didio: implement read-write lock

typedef struct {
    server_t current_server;
    server_t primary_server;

    sem_t metadata_mutex;
    metadata_t metadata;

    sem_t should_stop_mutex;
    bool should_stop;

    sem_t logging_mutex;

    connection_t *heartbeatSocket;
    sem_t heartbeatSocket_mutex;

    std::vector <connection_t*> heartbeatconnections;
    sem_t heartbeatvector_mutex;

    sem_t initialization_lock;

    sem_t deferred_requests_mutex;
    std::vector<request_t> deferred_requests;
} program_state_t;

/****************\
* Initialization *
\****************/
void state_init(uint32_t ip, uint16_t port, server_type_t type);

void wait_for_init();

void init_done();

std::vector<request_t> *acquire_deferred_requests();

void release_deferred_requests();

/*************\
* Should stop *
\*************/
bool should_stop();

void set_should_stop(bool value);

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

/***********\
* Heartbeat *
\***********/
connection_t** get_heartbeat_socket();

void set_heartbeat_socket(connection_t *heartbeatconnection);

void release_heartbeat_socket();

std::vector <connection_t*>* get_heartbeat_connections();

void release_heartbeat_connections();

#define LOG_SYNC(x) acquire_logging_mutex(); x; release_logging_mutex(); std::cerr.flush(); std::cout.flush()
// #define LOG_SYNC(x)
