#include "../include/reader.h"

#include <asm-generic/errno-base.h>
#include <cerrno>
#include <cinttypes>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define rnet_debug false

void print_reader(struct tcp_reader& reader) {
    // std::cerr << "Reader(sockfd=" << reader.sockfd << ", index=" << reader.index << ", length=" << reader.length << ")" << std::endl;
}

struct tcp_reader init_reader(int sockfd) {
    struct tcp_reader reader;
    reader.sockfd = sockfd;
    reader.index = 0;
    reader.length = 0;
    reader.buffer_switch = false;
    return reader;
}

uint8_t *curr_buf(struct tcp_reader& reader) {
    if (reader.buffer_switch) {
        return reader.a_buffer;
    } else {
        return reader.b_buffer;
    }
}

bool fill_buffer(struct tcp_reader& reader) {
    while (reader.index >= reader.length) {
        reader.buffer_switch = !reader.buffer_switch;
        uint8_t *buffer = curr_buf(reader);

        int read_size = read(reader.sockfd, buffer, RSIZE);
        if (read_size <= 0) {
            if (errno == EAGAIN) {
                continue;
            }
            std::cerr << "ERROR: [Reader.fill_buffer] read failed with errno = `" << errno << "`" << std::endl; 
            return false;
        }
        reader.index = 0;
        reader.length = (uint64_t) read_size;
        if (rnet_debug) {
            std::cerr << "Received data: (" << reader.length << ") [";
            for (uint64_t i = 0; i < reader.length; i++) {
                std::cerr << " " << std::hex << ((uint64_t) curr_buf(reader)[i]) << std::dec;
            }
            std::cerr << " ]" << std::endl;
        }
    }

    return true;
}

bool ready(struct tcp_reader& reader) {
    if (reader.index >= reader.length) {
        uint32_t error_code;
        uint32_t error_code_size = sizeof(error_code);
        getsockopt(reader.sockfd, SOL_SOCKET, SO_ERROR, &error_code, &error_code_size);
        if (error_code == 0) {
            return true;
        } else {
            return false;
        }
    }
    return true;
}

bool peek(struct tcp_reader& reader, uint8_t *val) {
    if (!fill_buffer(reader)) {
        return false;
    }
    *val = curr_buf(reader)[reader.index];
    return true;
}

bool step(struct tcp_reader& reader) {
    if (!fill_buffer(reader)) {
        return false;
    }
    reader.index++;
    return true;
}

bool read_u8(struct tcp_reader& reader, uint8_t *val) {
    uint8_t res;
    if (!peek(reader, &res)) {
        return false;
    }
    if (!step(reader)) {
        return false;
    }
    *val = res;
    return true;
}

bool read_u16(struct tcp_reader& reader, uint16_t *val) {
    uint16_t res = 0;
    uint8_t temp;
    for (int i = 0; i < 2; i++) {
        if (!read_u8(reader, &temp)) {
            return false;
        }
        res = res | (((uint16_t) temp) << (8 * i));
    }
    *val = res;
    return true;
}

bool read_u32(struct tcp_reader& reader, uint32_t *val) {
    uint32_t res = 0;
    uint8_t temp;
    for (int i = 0; i < 4; i++) {
        if (!read_u8(reader, &temp)) {
            return false;
        }
        res = res | (((uint32_t) temp) << (8 * i));
    }
    *val = res;
    return true;
}

bool read_u64(struct tcp_reader& reader, uint64_t *val) {
    uint64_t res = 0;
    uint8_t temp;
    for (int i = 0; i < 8; i++) {
        if (!read_u8(reader, &temp)) {
            return false;
        }
        res = res | (((uint64_t) temp) << (8 * i));
    }
    *val = res;
    return true;
}

bool read_char(struct tcp_reader& reader, char *val) {
    uint8_t res;
    if (!peek(reader, &res)) {
        return false;
    }
    if (!step(reader)) {
        return false;
    }
    *val = (char) res;
    return true;
}

bool read_string(struct tcp_reader& reader, std::string *val) {
    uint64_t len;
    char c;
    if (!read_u64(reader, &len)) {
        return true;
    }
    std::string str = std::string();
    str.reserve(len);
    for (uint64_t i = 0; i < len; i++) {
        if (!read_char(reader, &c)) {
            return false;
        }
        str.push_back(c);
    }
    *val = str;
    return true;
}

bool read_bytes(struct tcp_reader& reader, uint8_t *val, uint64_t length) {
    for (uint64_t i = 0; i < length; i++) {
        if (!peek(reader, val+i)) {
            return false;
        }
        if (!step(reader)) {
            return false;
        }
    }
    return true;
}

bool read_byte_array(struct tcp_reader& reader, uint8_t **val, uint64_t *length) {
    read_u64(reader, length);

    *val = (uint8_t*) malloc(sizeof(uint8_t)*(*length));
    if (val == NULL) {
        return false;
    }

    for (uint64_t i = 0; i < *length; i++) {
        if (!peek(reader, (*val)+i)) {
            free(*val);
            return false;
        }
        if (!step(reader)) {
            free(*val);
            return false;
        }
    }

    return true;
}
