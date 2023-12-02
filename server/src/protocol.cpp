#include "../include/protocol.h"
#include "../include/connection.h"
#include "../include/server.h"
#include <cstdint>
#include <cstdlib>

#define PACKET_VERSION 1
#define PACKET_TYPE_HANDSHAKE 0
#define PACKET_TYPE_DOWNLOAD 1
#define PACKET_TYPE_UPLOAD 2
#define PACKET_TYPE_DELETE 3
#define PACKET_TYPE_LIST_FILES 4
#define PACKET_TYPE_EXIT 5
    
typedef struct {
    uint16_t protocol_version;
    uint64_t packet_length;
    uint8_t packet_type;
} packet_header_t;

bool read_header(client_t *client, packet_header_t *header) {
    packet_header_t _header;
    if (!read_u16(client->connection->reader, &_header.protocol_version)) {
        return false;
    }
    if (!read_u64(client->connection->reader, &_header.packet_length)) {
        return false;
    }
    if (!read_u8(client->connection->reader, &_header.packet_type)) {
        return false;
    }
    *header = _header;
    return true;
}

bool handshake(client_t *client, std::string *username) {
    packet_header_t header;
    if (!read_header(client, &header)) {
        return false;
    }
    if (header.protocol_version != PACKET_VERSION ||
            header.packet_type != PACKET_TYPE_HANDSHAKE) {
        return false;
    }
    if (!read_string(client->connection->reader, username)) {
        return false;
    }
    return true;
}

bool receive_packet(client_t *client, packet_header_t *header, std::string *filename, uint64_t *length, uint8_t *bytes) {
    if (!read_header(client, header)) {
        return false;
    }
    if (header->protocol_version != PACKET_VERSION) {
        return false;
    }

    switch (header->packet_type) {
        case PACKET_TYPE_HANDSHAKE:
            return false;
            break;
        case PACKET_TYPE_DOWNLOAD:
            read_string(client->connection->reader, filename);
            break;
        case PACKET_TYPE_UPLOAD:
            read_string(client->connection->reader, filename);
            read_u64(client->connection->reader, length);
            bytes = (uint8_t*) malloc(sizeof(uint8_t)*(*length));
            read_bytes(client->connection->reader, bytes, *length);
            break;
        case PACKET_TYPE_DELETE:
            read_string(client->connection->reader, filename);
            break;
        case PACKET_TYPE_LIST_FILES:
            break;
        case PACKET_TYPE_EXIT:
            break;
        default:
            return false;
            break;
    }
    return true;
}

