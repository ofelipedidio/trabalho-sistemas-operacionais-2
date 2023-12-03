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

/*  ###########################
 *  #  Non-serialized access  #
 *  ########################### */

bool flush(struct tcp_writer& writer);

/*  ################
 *  #  Primitives  #
 *  ################ */

/* unsigned 8-bit integer */
bool write_u8(struct tcp_writer& writer, uint8_t value);

/* unsigned 16-bit integer */
bool write_u16(struct tcp_writer& writer, uint32_t value);

/* unsigned 32-bit integer */
bool write_u32(struct tcp_writer& writer, uint32_t value);

/* unsigned 64-bit integer */
bool write_u64(struct tcp_writer& writer, uint64_t value);

/* C 8-bit char */
bool write_char(struct tcp_writer& writer, char c);

/*  ##############
 *  #  Composed  #
 *  ############## */

/*
 * Writes the lengths of the string (u64) and length chars
 */
bool write_cstr(struct tcp_writer& writer, const char *string);

/*
 * Writes the lengths of the string (u64) and length chars
 */
bool write_string(struct tcp_writer& writer, std::string value);

/*
 * Writes len chars
 */
bool write_bytes(struct tcp_writer& writer, uint8_t *buf, uint64_t len);

/*
 * Writes the length (u64) and the length bytes
 */
bool write_byte_array(struct tcp_writer& writer, uint8_t *buf, uint64_t len);

#endif // !WRITER
