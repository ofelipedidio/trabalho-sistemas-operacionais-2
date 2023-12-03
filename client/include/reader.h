#ifndef READER
#define READER

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <sys/socket.h>

#define RSIZE 32

struct tcp_reader {
    int sockfd;
    uint8_t a_buffer[RSIZE];
    uint8_t b_buffer[RSIZE];
    uint64_t index;
    uint64_t length;
    bool buffer_switch;
};

void print_reader(struct tcp_reader& reader);

struct tcp_reader init_reader(int sockfd);

/*  ###########################
 *  #  Non-serialized access  #
 *  ########################### */

/*
 * Fills the buffer
 */
bool fill_buffer(struct tcp_reader& reader);

/*
 * Checks if the connection is still valid
 */
bool ready(struct tcp_reader& reader);

/*
 * Gets the first character from the stream, but doesn't pop it
 */
bool peek(struct tcp_reader& reader, uint8_t *val);

/*
 * Pops the first character out of the stream
 */
bool step(struct tcp_reader& reader);

/*  ################
 *  #  Primitives  #
 *  ################ */

/* unsigned 8-bit integer */
bool read_u8(struct tcp_reader& reader, uint8_t *val);

/* unsigned 16-bit integer */
bool read_u16(struct tcp_reader& reader, uint16_t *val);

/* unsigned 32-bit integer */
bool read_u32(struct tcp_reader& reader, uint32_t *val);

/* unsigned 64-bit integer */
bool read_u64(struct tcp_reader& reader, uint64_t *val);

/* C 8-bit char */
bool read_char(struct tcp_reader& reader, char *val);

/*  ##############
 *  #  Composed  #
 *  ############## */

/*
 * Reads the length *u64) and length chars after that to construct the string
 */
bool read_string(struct tcp_reader& reader, std::string *val);

/*
 * Reads length bytes and stores in val
 */
bool read_bytes(struct tcp_reader& reader, uint8_t *val, uint64_t length);

/*
 * Reads the length (u64) and length u8's into a malloc-allocated buffer
 */
bool read_byte_array(struct tcp_reader& reader, uint8_t **val, uint64_t *length);

#endif // !READER 

