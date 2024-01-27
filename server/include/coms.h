#pragma once

/******************************************\
* This module contains the code related to *
* the communication between servers.       *
\******************************************/

#include "connection.h"
#include "election.h"

typedef struct {
    connection_t inbound_connection;
    connection_t outbound_connection;
} coms_server_t;

typedef enum {
    req_transaction_start,
    req_transaction_end,
    req_fetch_metadata,
    req_hello,
} request_type_t;

typedef struct {
    request_type_t type;
} request_t;

typedef struct {
    uint16_t status;
    metadata_t metadata;
} response_t;

bool _coms_sync_execute_request(tcp_reader *reader, tcp_writer *writer, request_t request, response_t *out_response);

bool coms_thread_init();

