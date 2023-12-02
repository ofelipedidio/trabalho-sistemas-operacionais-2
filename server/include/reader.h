#ifndef READER
#define READER

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <sys/socket.h>

#define RSIZE 32

struct tcp_reader {
    int sockfd;
    uint8_t read_buffer[RSIZE];
    uint64_t read_start;
    uint64_t read_end;
};

struct tcp_reader init_reader(int sockfd);

bool ready(struct tcp_reader& reader);

bool peek(struct tcp_reader& reader, uint64_t offset, uint8_t *byte);

void advance(struct tcp_reader& reader, uint64_t amount);

bool read_char(struct tcp_reader& reader, char *c);

bool read_string(struct tcp_reader& reader, std::string *string);

bool read_u8(struct tcp_reader& reader, uint8_t *value);

bool read_u16(struct tcp_reader& reader, uint16_t *value);

bool read_u32(struct tcp_reader& reader, uint32_t *value);

bool read_u64(struct tcp_reader& reader, uint64_t *value);

bool read_bytes(struct tcp_reader& reader, uint8_t *val, uint64_t length);

#endif // !READER 
