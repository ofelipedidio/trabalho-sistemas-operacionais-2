#include "../include/coms.h"
#include <cstdlib>
#include <vector>

#define coms_exec(expr) \
    if (!expr) { \
        return 1; \
    }

#define STATUS_OK 0
#define STATUS_INVALID_REQUEST_TYPE 10
#define STATUS_NOT_IMPLEMENTED 11

bool _coms_sync_execute_request(coms_server_t *server, request_t request, response_t *out_response) {
    tcp_writer writer = server->outbound_connection.writer;
    tcp_reader reader = server->outbound_connection.reader;

    switch (request.type) {
        case req_transaction_start:
            coms_exec(write_u8(writer, 12));
            break;
        case req_transaction_end:
            coms_exec(write_u8(writer, 13));
            break;
        case req_fetch_metadata:
            coms_exec(write_u8(writer, 14));
            break;
    }

    coms_exec(flush(server->outbound_connection.writer));

    uint64_t metadata_length;
    uint64_t primary_index;
    server_t in_server;

    switch (request.type) {
        case req_transaction_start:
            coms_exec(read_u16(reader, &out_response->status));
            break;

        case req_transaction_end:
            coms_exec(read_u16(reader, &out_response->status));
            break;

        case req_fetch_metadata:
            out_response->metadata = { .servers = std::vector<server_t>() };
            coms_exec(read_u16(reader, &out_response->status));
            if (out_response->status == STATUS_OK) {
                coms_exec(read_u64(reader, &metadata_length));
                coms_exec(read_u64(reader, &primary_index));
                for (uint64_t i = 0; i < metadata_length; i++) {
                    coms_exec(read_u32(reader, &in_server.ip));
                    coms_exec(read_u16(reader, &in_server.port));
                    in_server.server_type = (i == primary_index) ? primary : secondary;
                    out_response->metadata.servers.push_back(in_server);
                }
            }
            break;
    }

    return true;
}

bool coms_handle_request(coms_server_t *server) {
    tcp_reader reader = server->inbound_connection.reader;
    tcp_writer writer = server->inbound_connection.writer;

    uint8_t request_type;
    coms_exec(read_u8(reader, &request_type));
    switch (request_type) {
        // req_transaction_start
        case 12:
            coms_exec(write_u16(writer, STATUS_NOT_IMPLEMENTED));
            break;
            // req_transaction_end
        case 13:
            coms_exec(write_u16(writer, STATUS_NOT_IMPLEMENTED));
            break;
            // req_fetch_metadata
        case 14:
            {
                uint64_t length = server->metadata.servers.size();
                uint64_t primary_index = length;
                for (uint64_t i = 0; i < length; i++) {
                    if (server->metadata.servers[i].server_type == primary) {
                        primary_index = i;
                        break;
                    }
                }
                coms_exec(write_u64(writer, length));
                coms_exec(write_u64(writer, primary_index));
                for (uint64_t i = 0; i < length; i++) {
                    coms_exec(write_u32(writer, server->metadata.servers[i].ip));
                    coms_exec(write_u16(writer, server->metadata.servers[i].port));
                }
            }
            break;
        default:
            coms_exec(write_u16(writer, STATUS_INVALID_REQUEST_TYPE));
            return false;
    }

    return true;
}

void coms_update_metadata(coms_server_t *server) {
    request_t request;
    request.type = req_fetch_metadata;
    response_t response;
    if (!_coms_sync_execute_request(server, request, &response)) {
        return;
    }
}

