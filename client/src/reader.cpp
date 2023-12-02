#include "../include/reader.h"

#include <cinttypes>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>

struct tcp_reader init_reader(int sockfd) {
    struct tcp_reader reader;
    reader.sockfd = sockfd;
    reader.read_start = 0;
    reader.read_end = 0;
    return reader;
}

bool ready(struct tcp_reader& reader) {
    if (reader.read_start == reader.read_end) {
        reader.read_start = 0;
        reader.read_end = 0;

        int read_size = read(reader.sockfd, reader.read_buffer, RSIZE-1);

        if (read_size <= 0) {
            return false;
        } else {
            reader.read_end += read_size;
            uint32_t error_code;
            uint32_t error_code_size = sizeof(error_code);
            getsockopt(reader.sockfd, SOL_SOCKET, SO_ERROR, &error_code, &error_code_size);
            if (error_code == 0) {
                return true;
            } else {
                return false;
            }
        }
    }
    return true;
}

bool peek(struct tcp_reader& reader, uint64_t offset, uint8_t *val) {
    if ((reader.read_end - reader.read_start) + offset + 1 >= RSIZE) {
        std::cerr << "[READER] Peak caused a buffer overflow" << std::endl;
        return false;
    }
    while (reader.read_start + offset >= reader.read_end) {
        int len = RSIZE - (reader.read_end - reader.read_start) - 1;
        int read_size = read(reader.sockfd, (reader.read_buffer + (reader.read_end % RSIZE)), len);
        if (read_size < 0) {
            std::cerr << "[READER] Peaked outside the buffer on a closed connection" << std::endl;
            return false;
        }
        reader.read_end += read_size;
    }
    *val = reader.read_buffer[(reader.read_start + offset) % RSIZE];
    return true;
}

bool step(struct tcp_reader& reader, uint64_t amount) {
    while (reader.read_start + amount > reader.read_end) {
        amount -= (reader.read_end - reader.read_start);
        reader.read_start = 0;
        reader.read_end = 0;
        int read_size = read(reader.sockfd, (void*) reader.read_buffer, RSIZE - 1);
        if (read_size < 0) {
            std::cerr << "[READER] Tried to advance beyond the buffer on a closed connection" << std::endl;
            return false;
        }
        reader.read_end += read_size;
    }
    reader.read_start += amount;
    return true;
}

bool read_char(struct tcp_reader& reader, char *val) {
    uint8_t res;
    if (!peek(reader, 0, &res)) {
        return false;
    }
    if (!step(reader, 1)) {
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
    return true;
}

bool read_u8(struct tcp_reader& reader, uint8_t *val) {
    uint8_t res;
    if (!peek(reader, 0, &res)) {
        return false;
    }
    if (!step(reader, 1)) {
        return false;
    }
    *val = res;
    return true;
}

bool read_u16(struct tcp_reader& reader, uint16_t *val) {
    uint16_t res = 0;
    uint8_t temp;
    for (int i = 0; i < 2; i++) {
        res = res << 8;
        if (!read_u8(reader, &temp)) {
            return false;
        }
        res = res | ((uint16_t) temp);
    }
    *val = res;
    return true;
}

bool read_u32(struct tcp_reader& reader, uint32_t *val) {
    uint32_t res = 0;
    uint8_t temp;
    for (int i = 0; i < 4; i++) {
        res = res << 8;
        if (!read_u8(reader, &temp)) {
            return false;
        }
        res = res | ((uint32_t) temp);
    }
    *val = res;
    return true;
}

bool read_u64(struct tcp_reader& reader, uint64_t *val) {
    uint64_t res = 0;
    uint8_t temp;
    for (int i = 0; i < 8; i++) {
        res = res << 8;
        if (!read_u8(reader, &temp)) {
            return false;
        }
        res = res | ((uint64_t) temp);
    }
    *val = res;
    return true;
}

bool read_bytes(struct tcp_reader& reader, uint8_t *val, uint64_t length) {
    uint8_t byte;
    for (uint64_t i = 0; i < length; i++) {
        if (!peek(reader, 0, &byte)) {
            return false;
        }
        if (!step(reader, 1)) {
            return false;
        }
        val[i] = byte;
    }
    return true;
}
