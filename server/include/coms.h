#pragma once

/******************************************\
* This module contains the code related to *
* the communication between servers.       *
\******************************************/

#include "connection.h"
#include "election.h"
#include <cstdint>

typedef struct {
    connection_t inbound_connection;
    connection_t outbound_connection;
} coms_server_t;

#define STATUS_OK 0
#define STATUS_INVALID_REQUEST_TYPE 10
#define STATUS_NOT_IMPLEMENTED 11
#define STATUS_TRANSACTION_ERROR 12

typedef enum {
    /*
     * Starts transaction
     *
     * If a transaction has already been initiated, returns STATUS_TRANSACTION_ERROR
     */
    req_transaction_start,
    
    /*
     * End transaction
     *
     * If a transaction has not already been initiated, returns STATUS_TRANSACTION_ERROR
     */
    req_transaction_end,
    
    /*
     * Request metadata
     */
    req_fetch_metadata,

    /*
     * Exchange connection start information
     */
    req_hello,

    /*
     * Exchange startup information
     */
    req_register,

    req_get_primary,
} request_type_t;

typedef struct {
    request_type_t type;
} request_t;

typedef struct {
    uint16_t status;
    metadata_t metadata;
    uint32_t ip;
    uint16_t port;
} response_t;

bool _coms_sync_execute_request(tcp_reader *reader, tcp_writer *writer, request_t request, response_t *out_response);

bool coms_thread_init();

void *heartbeat_listener_thread(void *args);

void *heartbeat_writer_thread(void *args);

bool heartbeat_thread_init();

bool primary_heartbeat_thread_init();
