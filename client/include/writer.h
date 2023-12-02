#ifndef WRITER
#define WRITER

#include <string>
#include <cstdint>

#define WSIZE 32

struct tcp_writer {
    int sockfd;
    uint8_t buffer[WSIZE];
    uint64_t index;
};

struct tcp_writer init_writer(int sockfd);

void flush(struct tcp_writer& writer);

void write_cstr(struct tcp_writer& writer, const char *string);

void write_bytes(struct tcp_writer& writer, uint8_t *buf, uint64_t len);

void write_char(struct tcp_writer& writer, char c);

void write_string(struct tcp_writer& writer, std::string value);

void write_u8(struct tcp_writer& writer, uint8_t value);

void write_u16(struct tcp_writer& writer, uint32_t value);

void write_u32(struct tcp_writer& writer, uint32_t value);

void write_u64(struct tcp_writer& writer, uint64_t value);

#endif // !WRITER
