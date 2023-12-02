#ifndef WRITER
#define WRITER

#include <string>
#include <cstdint>

template<int SIZE>
struct tcp_writer {
    int sockfd;
    uint8_t buffer[SIZE];
    uint64_t index;
};

template<int SIZE>
struct tcp_writer<SIZE> init_writer(int sockfd);

template<int SIZE>
void flush(struct tcp_writer<SIZE>& writer);

template<int SIZE>
void write_cstr(struct tcp_writer<SIZE>& writer, const char *string);

template<int SIZE>
void write_bytes(struct tcp_writer<SIZE>& writer, uint8_t *buf, uint64_t len);

template<int SIZE>
void write_char(struct tcp_writer<SIZE>& writer, char c);

template<int SIZE>
void write_string(struct tcp_writer<SIZE>& writer, std::string value);

template<int SIZE>
void write_u8(struct tcp_writer<SIZE>& writer, uint8_t value);

template<int SIZE>
void write_u16(struct tcp_writer<SIZE>& writer, uint32_t value);

template<int SIZE>
void write_u32(struct tcp_writer<SIZE>& writer, uint32_t value);

template<int SIZE>
void write_u64(struct tcp_writer<SIZE>& writer, uint64_t value);

#endif // !WRITER

