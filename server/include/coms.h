/******************************************\
* This module contains the code related to *
* the communication between servers.       *
\******************************************/

#include "connection.h"
#include "election.h"

typedef struct {
    connection_t inbound_connection;
    connection_t outbound_connection;
    metadata_t metadata;
} coms_server_t;

typedef enum {
    req_transaction_start,
    req_transaction_end,
    req_fetch_metadata,
} request_type_t;

typedef struct {
    request_type_t type;
} request_t;

typedef struct {
    uint16_t status;
    metadata_t metadata;
} response_t;


/*
 * Gets the metadata from the outbound server
 */
void update_metadata(coms_server_t *server);

