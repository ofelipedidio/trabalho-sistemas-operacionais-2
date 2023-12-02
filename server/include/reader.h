#ifndef READER
#define READER

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <sys/socket.h>

template<int SIZE>
struct tcp_reader {
    int sockfd;
    uint8_t read_buffer[SIZE];
    uint64_t read_start;
    uint64_t read_end;
};

template<int SIZE>
struct tcp_reader<SIZE> init_reader(int sockfd);

template<int SIZE>
bool ready(struct tcp_reader<SIZE>& reader);

template<int SIZE>
bool peek(struct tcp_reader<SIZE>& reader, uint64_t offset, uint8_t *byte);

template<int SIZE>
void advance(struct tcp_reader<SIZE>& reader, uint64_t amount);

template<int SIZE>
bool read_char(struct tcp_reader<SIZE>& reader, char *c);

template<int SIZE>
bool read_string(struct tcp_reader<SIZE>& reader, std::string *string);

template<int SIZE>
bool read_u8(struct tcp_reader<SIZE>& reader, uint8_t *value);

template<int SIZE>
bool read_u16(struct tcp_reader<SIZE>& reader, uint16_t *value);

template<int SIZE>
bool read_u32(struct tcp_reader<SIZE>& reader, uint32_t *value);

template<int SIZE>
bool read_u64(struct tcp_reader<SIZE>& reader, uint64_t *value);

template<int SIZE>
bool read_bytes(struct tcp_reader<SIZE>& reader, uint8_t *val, uint64_t length);

#endif // READER 

