#include "../include/protocol.h"
#include <cstdint>

bool request_handshake(connection_t *connection, std::string username, uint8_t *out_status) {
    // Send the request
    write_u16(connection->writer, PROTOCOL_VERSION);
    write_u8(connection->writer, PACKET_TYPE_HANDSHAKE);
    flush(connection->writer);


    // Receive the response
    uint16_t protocol_version;
    if (!read_u16(connection->reader, &protocol_version)) {
        return false;
    }
    if (protocol_version != PROTOCOL_VERSION) {
        return false;
    }
    if (!read_u8(connection->reader, out_status)) {
        return false;
    }

    return true;
}

bool request_download(connection_t *connection, std::string filename, uint8_t *out_status, uint8_t **out_buf, uint64_t *out_length) {
    // Send the request
    write_u16(connection->writer, PROTOCOL_VERSION);
    write_u8(connection->writer, PACKET_TYPE_DOWNLOAD);
    write_string(connection->writer, filename);
    flush(connection->writer);

    // Receive the response
    uint16_t protocol_version;
    if (!read_u16(connection->reader, &protocol_version)) {
        return false;
    }
    if (protocol_version != PROTOCOL_VERSION) {
        return false;
    }
    if (!read_u8(connection->reader, out_status)) {
        return false;
    }
    if (*out_status != STATUS_SUCCESS) {
        return true;
    }
    if (!read_u64(connection->reader, out_length)) {
        return false;
    }
    *out_buf = (uint8_t*) malloc(sizeof(uint8_t)*(*out_length));
    if (*out_buf == NULL) {
        return false;
    }
    if (!read_bytes(connection->reader, *out_buf, *out_length)) {
        free(out_buf);
        return false;
    }

    return true;
}

bool request_upload(connection_t *connection, std::string filename, uint8_t *buffer, uint64_t length, uint8_t *out_status) {
    // Send the request
    write_u16(connection->writer, PROTOCOL_VERSION);
    write_u8(connection->writer, PACKET_TYPE_UPLOAD);
    write_string(connection->writer, filename);
    write_bytes(connection->writer, buffer, length);
    flush(connection->writer);

    // Receive the response
    uint16_t protocol_version;
    if (!read_u16(connection->reader, &protocol_version)) {
        return false;
    }
    if (protocol_version != PROTOCOL_VERSION) {
        return false;
    }
    if (!read_u8(connection->reader, out_status)) {
        return false;
    }

    return true;
}

bool request_delete(connection_t *connection, std::string filename, uint8_t *out_status) {
    // Send the request
    write_u16(connection->writer, PROTOCOL_VERSION);
    write_u8(connection->writer, PACKET_TYPE_DELETE);
    write_string(connection->writer, filename);
    flush(connection->writer);

    // Receive the response
    uint16_t protocol_version;
    if (!read_u16(connection->reader, &protocol_version)) {
        return false;
    }
    if (protocol_version != PROTOCOL_VERSION) {
        return false;
    }
    if (!read_u8(connection->reader, out_status)) {
        return false;
    }

    return true;
}

bool request_list_files(connection_t *connection, uint8_t *out_status, std::vector<netfs::file_description_t> *out_files) {
    // Send the request
    write_u16(connection->writer, PROTOCOL_VERSION);
    write_u8(connection->writer, PACKET_TYPE_LIST_FILES);
    flush(connection->writer);

    // Receive the response
    uint16_t protocol_version;
    if (!read_u16(connection->reader, &protocol_version)) {
        return false;
    }
    if (protocol_version != PROTOCOL_VERSION) {
        return false;
    }
    if (!read_u8(connection->reader, out_status)) {
        return false;
    }
    if (*out_status != STATUS_SUCCESS) {
        return true;
    }
    uint64_t length;
    if (!read_u64(connection->reader, &length)) {
        return false;
    }
    std::vector<netfs::file_description_t> files = std::vector<netfs::file_description_t>();
    files.reserve(length);
    for (uint64_t i = 0; i < length; i++) {
        netfs::file_description_t file_description;
        if (!read_u64(connection->reader, &file_description.mac)) {
            return false;
        }
        if (!read_string(connection->reader, &file_description.filename)) {
            return false;
        }
        files.push_back(file_description);
    }
    *out_files = files;

    return true;
}

bool request_exit(connection_t *connection) {
    // Send the request
    write_u16(connection->writer, PROTOCOL_VERSION);
    write_u8(connection->writer, PACKET_TYPE_EXIT);
    flush(connection->writer);

    return true;
}

bool request_update(connection_t *connection, uint8_t *out_status, file_event_t *out_event) {
    // Send the request
    write_u16(connection->writer, PROTOCOL_VERSION);
    write_u8(connection->writer, PACKET_TYPE_UPDATE);
    flush(connection->writer);

    // Receive the response
    uint16_t protocol_version;
    if (!read_u16(connection->reader, &protocol_version)) {
        return false;
    }
    if (protocol_version != PROTOCOL_VERSION) {
        return false;
    }
    if (!read_u8(connection->reader, out_status)) {
        return false;
    }
    if (*out_status != STATUS_SUCCESS) {
        return true;
    }
    uint32_t event_type;
    std::string filename;
    if (!read_u32(connection->reader, &event_type)) {
        return false;
    }
    if (!read_string(connection->reader, &filename)) {
        return false;
    }
    switch (event_type) {
        case event_file_modified:
            out_event->type = event_file_modified;
            break;
        case event_file_deleted:
            out_event->type = event_file_deleted;
            break;
        default:
            std::cerr << "ERROR: [requesting update] Got unknown event type (" << event_type << ")" << std::endl;
            return false;
            break;
    }
    out_event->filename = filename;

    return true;
}

