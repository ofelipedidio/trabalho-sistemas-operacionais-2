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
#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>

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

bool handshake(connection_t *connection, std::string *username,bool* from_primary) {
    packet_header_t header;
    uint8_t temp;
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
    if(!read_u8(connection->reader,&temp)){
        return false;
    }
    (*from_primary) = temp == 1;
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
            read_byte_array(connection->reader, bytes, length);
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

bool respond_handshake_success(connection_t *connection) {
    if (!write_u16(connection->writer, PROTOCOL_VERSION)) {
        return false;
    }
    if (!write_u8(connection->writer, STATUS_SUCCESS)) {
        return false;
    }
    if (!flush(connection->writer)) {
        return false;
    }
    return true;
}

bool respond_handshake_fail(connection_t *connection) {
    if (!write_u16(connection->writer, PROTOCOL_VERSION)) {
        return false;
    }
    if (!write_u8(connection->writer, STATUS_FAIL)) {
        return false;
    }
    if (!flush(connection->writer)) {
        return false;
    }
    return true;
}

bool respond_download_success(connection_t *connection, uint8_t *buf, uint64_t length) {
    if (!write_u16(connection->writer, PROTOCOL_VERSION)) {
        return false;
    }
    if (!write_u8(connection->writer, STATUS_SUCCESS)) {
        return false;
    }
    if (!write_byte_array(connection->writer, buf, length)) {
        return false;
    }
    if (!flush(connection->writer)) {
        return false;
    }
    return true;
}

bool respond_download_fail(connection_t *connection) {
    if (!write_u16(connection->writer, PROTOCOL_VERSION)) {
        return false;
    }
    if (!write_u8(connection->writer, STATUS_FAIL)) {
        return false;
    }
    if (!flush(connection->writer)) {
        return false;
    }
    return true;
}

bool respond_upload_success(connection_t *connection) {
    if (!write_u16(connection->writer, PROTOCOL_VERSION)) {
        return false;
    }
    if (!write_u8(connection->writer, STATUS_SUCCESS)) {
        return false;
    }
    if (!flush(connection->writer)) {
        return false;
    }
    return true;
}

bool respond_upload_fail(connection_t *connection) {
    if (!write_u16(connection->writer, PROTOCOL_VERSION)) {
        return false;
    }
    if (!write_u8(connection->writer, STATUS_FAIL)) {
        return false;
    }
    if (!flush(connection->writer)) {
        return false;
    }
    return true;
}

bool respond_delete_success(connection_t *connection) {
    if (!write_u16(connection->writer, PROTOCOL_VERSION)) {
        return false;
    }
    if (!write_u8(connection->writer, STATUS_SUCCESS)) {
        return false;
    }
    if (!flush(connection->writer)) {
        return false;
    }
    return true;
}

bool respond_delete_fail(connection_t *connection) {
    if (!write_u16(connection->writer, PROTOCOL_VERSION)) {
        return false;
    }
    if (!write_u8(connection->writer, STATUS_FAIL)) {
        return false;
    }
    if (!flush(connection->writer)) {
        return false;
    }
    return true;
}

bool respond_list_files_success(connection_t *connection, std::vector<netfs::file_description_t> files) {
    if (!write_u16(connection->writer, PROTOCOL_VERSION)) {
        return false;
    }
    if (!write_u8(connection->writer, STATUS_SUCCESS)) {
        return false;
    }
    if (!write_u64(connection->writer, (uint64_t) files.size())) {
        return false;
    }
    for (std::size_t i = 0; i < files.size(); i++) {
        if (!write_u64(connection->writer, files[i].mtime)) {
            return false;
        }
        if (!write_u64(connection->writer, files[i].atime)) {
            return false;
        }
        if (!write_u64(connection->writer, files[i].ctime)) {
            return false;
        }
        if (!write_string(connection->writer, files[i].filename)) {
            return false;
        }
    }
    if (!flush(connection->writer)) {
        return false;
    }
    return true;
}

bool respond_list_files_fail(connection_t *connection) {
    if (!write_u16(connection->writer, PROTOCOL_VERSION)) {
        return false;
    }
    if (!write_u8(connection->writer, STATUS_FAIL)) {
        return false;
    }
    if (!flush(connection->writer)) {
        return false;
    }
    return true;
}

bool respond_update_some(connection_t *connection, file_event_t event) {
    if (!write_u16(connection->writer, PROTOCOL_VERSION)) {
        return false;
    }
    if (!write_u8(connection->writer, STATUS_SUCCESS)) {
        return false;
    }
    if (!write_u32(connection->writer, (uint32_t) event.type)) {
        return false;
    }
    if (!write_string(connection->writer, event.filename)) {
        return false;
    }
    if (!flush(connection->writer)) {
        return false;
    }
    return true;
}

bool respond_update_none(connection_t *connection) {
    if (!write_u16(connection->writer, PROTOCOL_VERSION)) {
        return false;
    }
    if (!write_u8(connection->writer, STATUS_EMPTY)) {
        return false;
    }
    if (!flush(connection->writer)) {
        return false;
    }
    return true;
}

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
    if (!write_u16(connection->writer, PROTOCOL_VERSION)) {
        return false;
    }
    if (!write_u8(connection->writer, PACKET_TYPE_UPDATE)) {
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
            std::cerr << "ERROR: [requesting update] Got unknown event type ("
                << event_type << ")" << std::endl;
            return false;
            break;
    }
    out_event->filename = filename;

    return true;
}
