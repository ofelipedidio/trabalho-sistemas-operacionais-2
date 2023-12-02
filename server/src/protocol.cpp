#include "../include/protocol.h"

#include <cstdint>
#include <cstdlib>
#include <cstdlib>
#include <vector>

#include "../include/connection.h"
#include "../include/client.h"
#include "../include/file_manager.h"
#include "../include/reader.h"
#include "../include/writer.h"

bool read_header(connection_t *connection, packet_header_t *header) {
    packet_header_t _header;
    if (!read_u16(connection->reader, &_header.protocol_version)) {
        return false;
    }
    if (!read_u8(connection->reader, &_header.packet_type)) {
        return false;
    }
    *header = _header;
    return true;
}

bool handshake(connection_t *connection, std::string *username) {
    packet_header_t header;
    if (!read_header(connection, &header)) {
        return false;
    }
    if (header.protocol_version != PROTOCOL_VERSION ||
            header.packet_type != PACKET_TYPE_HANDSHAKE) {
        return false;
    }
    if (!read_string(connection->reader, username)) {
        return false;
    }
    return true;
}

bool receive_packet(connection_t *connection, packet_header_t *header, std::string *filename, uint64_t *length, uint8_t **bytes) {
    if (!read_header(connection, header)) {
        return false;
    }
    if (header->protocol_version != PROTOCOL_VERSION) {
        return false;
    }

    switch (header->packet_type) {
        case PACKET_TYPE_HANDSHAKE:
            return false;
            break;
        case PACKET_TYPE_DOWNLOAD:
            read_string(connection->reader, filename);
            break;
        case PACKET_TYPE_UPLOAD:
            read_string(connection->reader, filename);
            read_u64(connection->reader, length);
            *bytes = (uint8_t*) malloc(sizeof(uint8_t)*(*length));
            read_bytes(connection->reader, *bytes, *length);
            break;
        case PACKET_TYPE_DELETE:
            read_string(connection->reader, filename);
            break;
        case PACKET_TYPE_LIST_FILES:
            break;
        case PACKET_TYPE_EXIT:
            break;
        case PACKET_TYPE_UPDATE:
            break;
        default:
            return false;
            break;
    }
    return true;
}

void respond_download_success(connection_t *connection, uint8_t *buf, uint64_t length) {
    write_u16(connection->writer, PROTOCOL_VERSION);
    write_u8(connection->writer, STATUS_SUCCESS);
    write_bytes(connection->writer, buf, length);
    flush(connection->writer);
}

void respond_download_fail(connection_t *connection) {
    write_u16(connection->writer, PROTOCOL_VERSION);
    write_u8(connection->writer, STATUS_FAIL);
    flush(connection->writer);
}

void respond_upload_success(connection_t *connection) {
    write_u16(connection->writer, PROTOCOL_VERSION);
    write_u8(connection->writer, STATUS_SUCCESS);
    flush(connection->writer);
}

void respond_upload_fail(connection_t *connection) {
    write_u16(connection->writer, PROTOCOL_VERSION);
    write_u8(connection->writer, STATUS_FAIL);
    flush(connection->writer);
}

void respond_delete_success(connection_t *connection) {
    write_u16(connection->writer, PROTOCOL_VERSION);
    write_u8(connection->writer, STATUS_SUCCESS);
    flush(connection->writer);
}

void respond_delete_fail(connection_t *connection) {
    write_u16(connection->writer, PROTOCOL_VERSION);
    write_u8(connection->writer, STATUS_FAIL);
    flush(connection->writer);
}

void respond_list_files_success(connection_t *connection, std::vector<FileManager::file_description_t> files) {
    write_u16(connection->writer, PROTOCOL_VERSION);
    write_u8(connection->writer, STATUS_SUCCESS);
    write_u64(connection->writer, (uint64_t) files.size());
    for (std::size_t i = 0; i < files.size(); i++) {
        write_u64(connection->writer, files[i].mac);
        write_string(connection->writer, files[i].filename);
    }
    flush(connection->writer);
}

void respond_list_files_fail(connection_t *connection) {
    write_u16(connection->writer, PROTOCOL_VERSION);
    write_u8(connection->writer, STATUS_FAIL);
    flush(connection->writer);
}

void respond_update_some(connection_t *connection, file_event_t event) {
    write_u16(connection->writer, PROTOCOL_VERSION);
    write_u8(connection->writer, STATUS_EMPTY);
    write_u32(connection->writer, (uint32_t) event.type);
    write_string(connection->writer, event.filename);
    flush(connection->writer);
}

void respond_update_none(connection_t *connection) {
    write_u16(connection->writer, PROTOCOL_VERSION);
    write_u8(connection->writer, STATUS_FAIL);
    flush(connection->writer);
}


