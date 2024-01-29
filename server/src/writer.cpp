#include "../include/writer.h"
#include <cerrno>
#include <sys/socket.h>
#include <cstdint>
#include <cstdio>
#include <iostream>

struct tcp_writer init_writer(int sockfd) {
    struct tcp_writer reader;
    reader.sockfd = sockfd;
    reader.index = 0;
    return reader;
}

bool flush(struct tcp_writer& writer) {
    if (false) {
        std::cerr << "Sent data: (" << writer.index << ") [";
        for (uint64_t i = 0; i < writer.index; i++) {
            std::cerr << " " << std::hex << ((uint64_t) writer.buffer[i]) << std::dec;
        }
        std::cerr << " ]" << std::endl;
    }

    int bytes_sent = send(writer.sockfd, writer.buffer, writer.index, 0);
    if (bytes_sent < 0) {
        std::cerr << "ERROR: [flushing writer] send failed with errno = `" << errno << "`" << std::endl;
        return false;
    }
    writer.index = 0;
    return true;
}

bool write_u8(struct tcp_writer& writer, uint8_t value) {
    if (writer.index >= WSIZE) {
        if (!flush(writer)) {
            return false;
        }
    }
    writer.buffer[writer.index++] = value;
    return true;
}

bool write_u16(struct tcp_writer& writer, uint32_t value) {
    for (int i = 0; i < 2; i++) {
        uint16_t temp = value & 0xFF;
        value = value >> 8;
        if (!write_u8(writer, (uint8_t) temp)) {
            return false;
        }
    }
    return true;
}

bool write_u32(struct tcp_writer& writer, uint32_t value) {
    for (int i = 0; i < 4; i++) {
        uint32_t temp = value & 0xFF;
        value = value >> 8;
        if (!write_u8(writer, (uint8_t) temp)) {
            return false;
        }
    }
    return true;
}

bool write_u64(struct tcp_writer& writer, uint64_t value) {
    for (int i = 0; i < 8; i++) {
        uint64_t temp = value & 0xFF;
        value = value >> 8;
        if (!write_u8(writer, (uint8_t) temp)) {
            return false;
        }
    }
    return true;
}

bool write_char(struct tcp_writer& writer, char c) {
    if (writer.index >= WSIZE) {
        if (!flush(writer)) {
            return false;
        }
    }
    writer.buffer[writer.index++] = (uint8_t) c;
    return true;
}

bool write_cstr(struct tcp_writer& writer, const char *string) {
    uint64_t len;
    for (len = 0; string[len] != '\0'; len++) {}

    if (!write_u64(writer, len)) {
        return false;
    }
    for (uint64_t i = 0; i < len; i++) {
        if (!write_char(writer, string[i])) {
            return false;
        }
    }
    return true;
}

bool write_string(struct tcp_writer& writer, std::string value) {
    return write_cstr(writer, value.c_str());
}

bool write_bytes(struct tcp_writer& writer, uint8_t *buf, uint64_t len) {
    for (uint64_t i = 0; i < len; i++) {
        if (!write_u8(writer, buf[i])) {
            return false;
        }
    }
    return true;
}

bool write_byte_array(struct tcp_writer& writer, uint8_t *buf, uint64_t len) {
    if (!write_u64(writer, len)) {
        return false;
    }
    for (uint64_t i = 0; i < len; i++) {
        if (!write_u8(writer, buf[i])) {
            return false;
        }
    }
    return true;
}

