#include <atomic>
#include <cstdint>
#include <iostream>
#include <optional>
#include <pthread.h>
#include <queue>
#include <string>
#include <strings.h>
#include <vector>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "../include/net_protocol.h"

#define WRITER_COUNT 10
#define WRITER_BUFFER_SIZE 256

namespace Network {
    int sockfd[WRITER_COUNT];
    uint8_t buffer[WRITER_BUFFER_SIZE][WRITER_COUNT];
    uint64_t index[WRITER_COUNT];
    bool busy[WRITER_COUNT];

    sem_t mutex, available;

    void init() {
        memset(busy, false, WRITER_COUNT);
        sem_init(&mutex, 0, 1);
        sem_init(&available, 0, WRITER_COUNT);
    }

    int get_writer(int _sockfd) {
        sem_wait(&available);
        sem_wait(&mutex);
        int writer_id = -1;
        for (int i = 0; i < WRITER_COUNT; i++) {
            if (!busy[i]) {
                writer_id = i;
                busy[writer_id] = true;
                sockfd[writer_id] = _sockfd;
                index[writer_id] = 0;
                break;
            }
        }
        sem_post(&mutex);
        return writer_id;
    }

    void flush_writer(int writer_id) {
        send(sockfd[writer_id], buffer[writer_id], index[writer_id], 0);
        index[writer_id] = 0;
    }

    void release_writer(int writer_id) {
        if (index[writer_id] > 0) {
            flush_writer(writer_id);
        }
        busy[writer_id] = false;
        sem_post(&available);
    }

    void write(int writer_id, uint8_t *buf, uint64_t len) {
        while (index[writer_id] + len >= WRITER_BUFFER_SIZE) {
            auto write_len = WRITER_BUFFER_SIZE - index[writer_id];
            memcpy(buffer[writer_id]+index[writer_id], buf, write_len);
            flush_writer(writer_id);
            buf += write_len;
            len -= write_len;
        }
        memcpy(buffer[writer_id]+index[writer_id], buf, len);
        index[writer_id] += len;
    }

    void write(int writer_id, uint8_t value) {
        write(writer_id, &value, 1);
    }

    void write(int writer_id, uint16_t value) {
        write(writer_id, (uint8_t*) &value, 2);
    }

    void write(int writer_id, uint32_t value) {
        write(writer_id, (uint8_t*) &value, 4);
    }

    void write(int writer_id, message_string value) {
        write(writer_id, value.length);
        write(writer_id, (uint8_t*) value.content, value.length);
    }

    void write(int writer_id, struct message_header value) {
        write(writer_id, value.protocol_version);
        write(writer_id, value.protocol_version);
        write(writer_id, value.message_type);
        write(writer_id, value.username_length);
        write(writer_id, value.username_content, value.username_length);
    }

    void write(int writer_id, struct message_download value) {
        write(writer_id, value.header);
        write(writer_id, value.filename);
    }

    namespace __internal {

// Auxiliar wrapper on send_message and send functions
#define _send_message(sockfd, value) ({auto ret = send_message(sockfd, value); if (ret == -1) { return ret; }; ret;})
#define _send(sockfd, value, length, flags) ({auto ret = send(sockfd, value, length, flags); if (ret == -1) { return ret; }; ret;})

        ssize_t send_message(int sockfd, uint8_t value) {
            return send(sockfd, &value, sizeof(uint8_t), 0);
        }

        ssize_t send_message(int sockfd, uint16_t value) {
            return send(sockfd, &value, sizeof(uint16_t), 0);
        }

        ssize_t send_message(int sockfd, uint32_t value) {
            return send(sockfd, &value, sizeof(uint32_t), 0);
        }

        ssize_t send_message(int sockfd, struct message_string value) {
            ssize_t len = 0;
            len += _send_message(sockfd, value.length);
            len += _send(sockfd, value.content, value.length, 0);
            return len;
        }

        ssize_t send_message(int sockfd, struct message_header value) {
            ssize_t len = 0;
            len += _send_message(sockfd, value.protocol_version);
            len += _send_message(sockfd, value.message_type);
            len += _send_message(sockfd, value.username_length);
            len += _send(sockfd, value.username_content, value.username_length, 0);
            return len;
        }

        ssize_t send_message(int sockfd, struct message_download value) {
            ssize_t len = 0;
            len += _send_message(sockfd, value.header);
            len += _send_message(sockfd, value.filename);
            return len;
        }
    }
}
