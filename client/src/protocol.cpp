#include "../include/protocol.h"
#include <cstdint>
#include <ostream>

bool request_handshake(connection_t *connection, std::string username, bool from_primary,
        uint8_t *out_status) {
    // Send the request
    if (!write_u16(connection->writer, PROTOCOL_VERSION)) {
        return false;
    }
    if (!write_u8(connection->writer, PACKET_TYPE_HANDSHAKE)) {
        return false;
    }
    if (!write_string(connection->writer, username)) {
        return false;
    }
    if (!write_u8(connection->writer, from_primary?1:0)) {
        return false;
    }
    if (!flush(connection->writer)) {
        return false;
    }
    
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

bool request_download(connection_t *connection, std::string filename,
        uint8_t *out_status, uint8_t **out_buf,
        uint64_t *out_length) {
    // Send the request
    if (!write_u16(connection->writer, PROTOCOL_VERSION)) {
        return false;
    }
    if (!write_u8(connection->writer, PACKET_TYPE_DOWNLOAD)) {
        return false;
    }
    if (!write_string(connection->writer, filename)) {
        return false;
    }
    if (!flush(connection->writer)) {
        return false;
    }

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
    if (!read_byte_array(connection->reader, out_buf, out_length)) {
        return false;
    }

    return true;
}

bool request_upload(connection_t *connection, std::string filename,
        uint8_t *buffer, uint64_t length, uint8_t *out_status) {
    // Send the request
    if (!write_u16(connection->writer, PROTOCOL_VERSION)) {
        return false;
    }
    if (!write_u8(connection->writer, PACKET_TYPE_UPLOAD)) {
        return false;
    }
    if (!write_string(connection->writer, filename)) {
        return false;
    }
    if (!write_byte_array(connection->writer, buffer, length)) {
        return false;
    }
    if (!flush(connection->writer)) {
        return false;
    }

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

bool request_delete(connection_t *connection, std::string filename,
        uint8_t *out_status) {
    // Send the request
    if (!write_u16(connection->writer, PROTOCOL_VERSION)) {
        return false;
    }
    if (!write_u8(connection->writer, PACKET_TYPE_DELETE)) {
        return false;
    }
    if (!write_string(connection->writer, filename)) {
        return false;
    }
    if (!flush(connection->writer)) {
        return false;
    }

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

bool request_list_files(connection_t *connection, uint8_t *out_status,
        std::vector<netfs::file_description_t> *out_files) {
    // Send the request
    if (!write_u16(connection->writer, PROTOCOL_VERSION)) {
        return false;
    }
    if (!write_u8(connection->writer, PACKET_TYPE_LIST_FILES)) {
        return false;
    }
    if (!flush(connection->writer)) {
        return false;
    }

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
    std::vector<netfs::file_description_t> files =
        std::vector<netfs::file_description_t>();
    files.reserve(length);
    for (uint64_t i = 0; i < length; i++) {
        netfs::file_description_t file_description;
        if (!read_u64(connection->reader, &file_description.mtime)) {
            return false;
        }
        if (!read_u64(connection->reader, &file_description.atime)) {
            return false;
        }
        if (!read_u64(connection->reader, &file_description.ctime)) {
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
    if (!write_u16(connection->writer, PROTOCOL_VERSION)) {
        return false;
    }
    if (!write_u8(connection->writer, PACKET_TYPE_EXIT)) {
        return false;
    }
    if (!flush(connection->writer)) {
        return false;
    }

    return true;
}

bool request_update(connection_t *connection, uint8_t *out_status,
        file_event_t *out_event) {
    // Send the request
    std::cerr << "\033[33m - 13 - \033[0m" << std::endl;
    if (!write_u16(connection->writer, PROTOCOL_VERSION)) {
        return false;
    }
    std::cerr << "\033[33m - 14 - \033[0m" << std::endl;
    if (!write_u8(connection->writer, PACKET_TYPE_UPDATE)) {
        return false;
    }
    std::cerr << "\033[33m - 15 - \033[0m" << std::endl;
    if (!flush(connection->writer)) {
        return false;
    }

    // Receive the response
    uint16_t protocol_version;
    std::cerr << "\033[33m - 16 - \033[0m" << std::endl;
    if (!read_u16(connection->reader, &protocol_version)) {
        return false;
    }
    std::cerr << "\033[33m - 17 - \033[0m" << std::endl;
    if (protocol_version != PROTOCOL_VERSION) {
        return false;
    }
    std::cerr << "\033[33m - 18 - \033[0m" << std::endl;
    if (!read_u8(connection->reader, out_status)) {
        return false;
    }
    std::cerr << "\033[33m - 19 - \033[0m" << std::endl;
    if (*out_status != STATUS_SUCCESS) {
        return true;
    }
    std::cerr << "\033[33m - 20 - \033[0m" << std::endl;
    uint32_t event_type;
    std::string filename;
    if (!read_u32(connection->reader, &event_type)) {
        return false;
    }
    std::cerr << "\033[33m - 21 - \033[0m" << std::endl;
    if (!read_string(connection->reader, &filename)) {
        return false;
    }
    std::cerr << "\033[33m - 22 - \033[0m" << std::endl;
    switch (event_type) {
        case event_file_modified:
            std::cerr << "\033[33m - 23 - \033[0m" << std::endl;
            out_event->type = event_file_modified;
            break;
        case event_file_deleted:
            std::cerr << "\033[33m - 24 - \033[0m" << std::endl;
            out_event->type = event_file_deleted;
            break;
        default:
            std::cerr << "\033[33m - 25 - \033[0m" << std::endl;
            std::cerr << "ERROR: [requesting update] Got unknown event type ("
                << event_type << ")" << std::endl;
            return false;
            break;
    }
    std::cerr << "\033[33m - 26 - \033[0m" << std::endl;
    out_event->filename = filename;
    std::cerr << "\033[33m - 27 - \033[0m" << std::endl;

    return true;
}
