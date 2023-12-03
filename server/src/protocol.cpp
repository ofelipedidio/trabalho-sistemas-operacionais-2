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
    std::cerr << "[" << syscall(__NR_gettid) << "] " << "b1" << std::endl;
    if (!read_u16(connection->reader, &_header.protocol_version)) {
    std::cerr << "[" << syscall(__NR_gettid) << "] " << "b2" << std::endl;
        return false;
    }
    std::cerr << "[" << syscall(__NR_gettid) << "] " << "b3" << std::endl;
    if (!read_u8(connection->reader, &_header.packet_type)) {
    std::cerr << "[" << syscall(__NR_gettid) << "] " << "b4" << std::endl;
        return false;
    }
    std::cerr << "[" << syscall(__NR_gettid) << "] " << "b5" << std::endl;
    *header = _header;
    std::cerr << "[" << syscall(__NR_gettid) << "] " << "b6" << std::endl;
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
    std::cerr << "[" << syscall(__NR_gettid) << "] " << "a1" << std::endl;
    if (!read_header(connection, header)) {
        std::cerr << "[" << syscall(__NR_gettid) << "] " << "a2" << std::endl;
        return false;
    }
    std::cerr << "[" << syscall(__NR_gettid) << "] " << "a3" << std::endl;
    if (header->protocol_version != PROTOCOL_VERSION) {
    std::cerr << "[" << syscall(__NR_gettid) << "] " << "a4" << std::endl;
        return false;
    }

    std::cerr << "[" << syscall(__NR_gettid) << "] " << "a5" << std::endl;
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
    std::cerr << "[" << syscall(__NR_gettid) << "] " << "a6" << std::endl;
            break;
        case PACKET_TYPE_EXIT:
            break;
        case PACKET_TYPE_UPDATE:
            break;
        default:
            return false;
            break;
    }
    std::cerr << "[" << syscall(__NR_gettid) << "] " << "a7" << std::endl;
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


