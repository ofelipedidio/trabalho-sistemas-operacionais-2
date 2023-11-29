#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <pthread.h>
#include <string>
#include <sys/socket.h>

#include "../../client/include/logger.h"

/*  ##################
 *  #     READER     #
 *  ################## */

template<int SIZE>
struct tcp_reader {
    int sockfd;
    uint8_t read_buffer[SIZE];
    uint64_t read_start;
    uint64_t read_end;
};

template<int SIZE>
struct tcp_reader<SIZE> init_reader(int sockfd) {
    struct tcp_reader<SIZE> reader;
    reader.sockfd = sockfd;
    reader.read_start = 0;
    reader.read_end = 0;
    return reader;
}

template<int SIZE>
bool ready(struct tcp_reader<SIZE>& reader) {
    if (reader.read_start == reader.read_end) {
        reader.read_start = 0;
        reader.read_end = 0;

        int read_size = read(reader.sockfd, reader.read_buffer, SIZE-1);

        log_debug("[READER] " log_value(read_size));
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
    return false;
}

template<int SIZE>
uint8_t peek(struct tcp_reader<SIZE>& reader, uint64_t offset) {
    if ((reader.read_end - reader.read_start) + offset + 1 >= SIZE) {
        // Buffer overflow
        // TODO - Didio: handle exception (low priority, this never happens with SIZE > 2)
        log_debug("[READER] Peak caused a buffer overflow");
        pthread_exit(NULL);
        exit(EXIT_FAILURE);
        return 0;
    }
    while (reader.read_start + offset >= reader.read_end) {
        int len = SIZE - (reader.read_end - reader.read_start) - 1;
        int read_size = read(reader.sockfd, (reader.read_buffer + (reader.read_end % SIZE)), len);
        if (read_size < 0) {
            // TODO - Didio: URGENT: handle read_size == -1 (error)
            // Temporary fix, this is just a way to debug it, if it occurs
            log_debug("[READER] Peaked outside the buffer on a closed connection");
            pthread_exit(NULL);
            exit(EXIT_FAILURE);
            return 0;
        }
        reader.read_end += read_size;
    }
    return reader.read_buffer[(reader.read_start + offset) % SIZE];
}

template<int SIZE>
void advance(struct tcp_reader<SIZE>& reader, uint64_t amount) {
    while (reader.read_start + amount > reader.read_end) {
        amount -= (reader.read_end - reader.read_start);
        reader.read_start = reader.read_end;
        int len = SIZE - 1;
        int read_size = read(reader.sockfd, (void*) (reader.read_buffer + (reader.read_end % SIZE)), len);
        reader.read_end += read_size;
    }
    reader.read_start += amount;
}

/*  ##################
 *  #     WRITER     #
 *  ################## */

template<int SIZE>
struct tcp_writer {
    int sockfd;
    uint8_t buffer[SIZE];
    uint64_t index;
};

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
void write(struct tcp_writer<SIZE>& writer, const char *string) {
    int i = 0;
    while (string[i] != '\0') {
        if (writer.index >= SIZE) {
            flush(writer);
        }
        writer.buffer[writer.index++] = string[i++];
    }
}

template<int SIZE>
void write(struct tcp_writer<SIZE>& writer, uint8_t *buf, uint64_t len) {
    long unsigned int i = 0;
    while (i < len) {
        if (writer.index >= SIZE) {
            flush(writer);
        }
        writer.buffer[writer.index++] = buf[i++];
    }
}

template<int SIZE>
void write(struct tcp_writer<SIZE>& writer, uint64_t value) {
    // Way bigger than uint64_t can be
    char buffer[32];
    sprintf(buffer, "%ld", value);
    write(writer, buffer);
}

template<int SIZE>
void write_hex(struct tcp_writer<SIZE>& writer, uint64_t value) {
    // Way bigger than uint64_t can be
    char buffer[32];
    sprintf(buffer, "%lx", value);
    write(writer, buffer);
}

template<int SIZE>
void write(struct tcp_writer<SIZE>& writer, std::string value) {
    write(writer, value.c_str());
}

