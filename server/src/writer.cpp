#include "../include/writer.h"
#include <cstdint>
#include <cstdio>

template<int SIZE>
struct tcp_writer<SIZE> init_writer(int sockfd) {
    struct tcp_writer<SIZE> reader;
    reader.sockfd = sockfd;
    reader.index = 0;
    return reader;
}

template<int SIZE>
void flush(struct tcp_writer<SIZE>& writer) {
    send(writer.sockfd, writer.buffer, writer.index, 0);
    writer.index = 0;
}

template<int SIZE>
void write_char(struct tcp_writer<SIZE>& writer, char c) {
    if (writer.index + 1 >= SIZE) {
        flush(writer);
    }
    writer.buffer[writer.index++] = (uint8_t) c;
}

template<int SIZE>
void write_cstr(struct tcp_writer<SIZE>& writer, const char *string) {
    uint64_t len;
    for (len = 0; string[len] == '\0'; len++) {}

    write_u64(writer, len);
    for (uint64_t i = 0; i < len; i++) {
        write_char(writer, string[i]);
    }
}

template<int SIZE>
void write_string(struct tcp_writer<SIZE>& writer, std::string value) {
    write_cstr(writer, value.c_str());
}

template<int SIZE>
void write_bytes(struct tcp_writer<SIZE>& writer, uint8_t *buf, uint64_t len) {
    write_u64(writer, len);
    int i = 0;
    while (i < len) {
        if (writer.index >= SIZE) {
            flush(writer);
        }
        writer.buffer[writer.index++] = buf[i++];
    }
}

template<int SIZE>
void write_u8(struct tcp_writer<SIZE>& writer, uint8_t value) {
    if (writer.index + 1 >= SIZE) {
        flush(writer);
    }
    writer.buffer[writer.index++] = value;
}

template<int SIZE>
void write_u16(struct tcp_writer<SIZE>& writer, uint32_t value) {
    for (int i = 0; i < 2; i++) {
        uint16_t temp = value & 0xFF00; // Get upper bits
        temp = temp >> 8;
        value = value << 8;
        write_u8(writer, (uint8_t) temp);
    }
}

template<int SIZE>
void write_u32(struct tcp_writer<SIZE>& writer, uint32_t value) {
    for (int i = 0; i < 4; i++) {
        uint32_t temp = value & 0xFF000000; // Get upper bits
        temp = temp >> 24;
        value = value << 8;
        write_u8(writer, (uint8_t) temp);
    }
}

template<int SIZE>
void write_u64(struct tcp_writer<SIZE>& writer, uint64_t value) {
    for (int i = 0; i < 8; i++) {
        uint64_t temp = value & 0xFF00000000000000; // Get upper bits
        temp = temp >> 56;
        value = value << 8;
        write_u8(writer, (uint8_t) temp);
    }
}

