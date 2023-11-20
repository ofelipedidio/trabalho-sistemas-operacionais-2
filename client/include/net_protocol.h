#include <atomic>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <optional>
#include <ostream>
#include <pthread.h>
#include <queue>
#include <string>
#include <strings.h>
#include <system_error>
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
#include <cstdint>

#include "logger.h"

// Protocol version transmitted on the packets' headers
#define PROTOCOL_VERSION 1
#define FRAME_SIZE 1024

/*
 * Request (generic)
 *
 * | 0 1 2 3 4 5 6 7 8 9 A B C D E F | 0 1 2 3 4 5 6 7 8 9 A B C D E F |
 * | -------------- | -------------- | -------------- | -------------- |
 * | PROTOC_VERSION |  MESSAGE_TYPE  |  UNAME_LENGTH  |   USERNAME...  |
 * | -------------- | -------------- | -------------- | -------------- |
 * |          PAYLOAD_LENGTH         |                |                |
 * | -------------- | -------------- | -------------- | -------------- |
 *
 * - PROTOC_VERSION (8 bits)            : The protocol version
 * - MESSAGE_TYPE   (8 bits)            : The message type
 * - UNAME_LENGTH   (8 bits)            : The length of the username
 * - USERNAME       (UNAME_LENGTH bytes): The username content. 
 * - PAYLOAD_LENGTH (16 bits)           : The length of request payload
 *                                        (everything after this field)
 */

/*
 * Response (generic)
 *
 * | 0 1 2 3 4 5 6 7 8 9 A B C D E F | 0 1 2 3 4 5 6 7 8 9 A B C D E F |
 * | -------------- | -------------- | -------------- | -------------- |
 * | PROTOC_VERSION |  STATUS_CODE   |          PAYLOAD_LENGTH         |
 * | -------------- | -------------- | -------------- | -------------- |
 *
 * - PROTC_VERSION  (8 bits) : The protocol version
 * - STATUS_CODE    (8 bits) : The status code
 * - PAYLOAD_LENGTH (16 bits): The length of request payload
 *                             (everything after this field)
 *
 * Status codes
 *
 * | CODE | MESSAGE                           | REQUEST HANDLED | CONNECTION |
 * | ---- | --------------------------------- | --------------- | ---------- |
 * | 0x00 | Ok                                | Yes             | Ready      |
 * | ---- | --------------------------------- | --------------- | ---------- |
 * | 0x10 | Invalid request                   | No              | Ready      |
 * | 0x11 | Invalid message type              | No              | Ready      |
 * | 0x12 | Invalid username length           | No              | Ready      |
 * | 0x13 | Invalid filename length           | No              | Ready      |
 * | 0x14 | Invalid timestamp                 | No              | Ready      |
 * | ---- | --------------------------------- | --------------- | ---------- |
 * | 0x20 | Unrecoverable error               | No              | Reset      |
 * | 0x21 | Unsupported protocol version      | No              | Close      |
 * | ---- | --------------------------------- | --------------- | ---------- |
 *
 */

/*
 * Request (list_files)
 *
 * | 0 1 2 3 4 5 6 7 8 9 A B C D E F | 0 1 2 3 4 5 6 7 8 9 A B C D E F |
 * | -------------- | -------------- | -------------- | -------------- |
 * | PROTOC_VERSION |      0x01      |  UNAME_LENGTH  |   USERNAME...  |
 * | -------------- | -------------- | -------------- | -------------- |
 *
 *
 * Response (list_files)
 *
 * | 0 1 2 3 4 5 6 7 8 9 A B C D E F | 0 1 2 3 4 5 6 7 8 9 A B C D E F |
 * | -------------- | -------------- | -------------- | -------------- |
 * | PROTOC_VERSION |  STATUS_CODE   |          PAYLOAD_LENGTH         |
 * | -------------- | -------------- | -------------- | -------------- |
 *
 * | 0 1 2 3 4 5 6 7 8 9 A B C D E F | 0 1 2 3 4 5 6 7 8 9 A B C D E F |
 * | -------------- | -------------- | -------------- | -------------- |
 * |         FILENAME_LENGTH         |             FILENAME            |
 * | -------------- | -------------- | -------------- | -------------- |
 *
 */

/*
 *
 */

/*
 * Request (upload)
 *
 * | 0 1 2 3 4 5 6 7 8 9 A B C D E F | 0 1 2 3 4 5 6 7 8 9 A B C D E F |
 * | -------------- | -------------- | -------------- | -------------- |
 * | PROTOC_VERSION |      0x02      |  UNAME_LENGTH  |   USERNAME...  |
 * | -------------- | -------------- | -------------- | -------------- |
 * |                       FILE_UPDATE_TIMESTAMP                       |
 * | -------------- | -------------- | -------------- | -------------- |
 * |                       FILE_UPDATE_TIMESTAMP                       |
 * | -------------- | -------------- | -------------- | -------------- |
 * |         FILENAME_LENGTH         |           FILENAME...           |
 * | -------------- | -------------- | -------------- | -------------- |
 *
 * - FILE_UPDATE_TIMESTAMP (64 bits)              : The time when the file was created or modified
 * - FILENAME_LENGTH       (16 bits)              : The length of the filename (1 <= FILENAME_LENGTH <= 1024)
 * - FILENAME              (FILENAME_LENGTH bytes): The filename characters in ASCII
 *
 * After the upload request is sent, several upload frames can be sent.
 * The last upload frame is the only frame on the stream that has length 0.
 *
 * | 0 1 2 3 4 5 6 7 8 9 A B C D E F | 0 1 2 3 4 5 6 7 8 9 A B C D E F |
 * | -------------- | -------------- | -------------- | -------------- |
 * |          FRAME_LENGTH           |        FRAME_CONTENT...         |
 * | -------------- | -------------- | -------------- | -------------- |
 *
 * - FRAME_LENGTH (16 bits)            : The length of the frame
 * - FRAME_CONTENT (FRAME_LENGTH bytes): The bytes from the frame
 *
 * Response (upload)
 *
 */

namespace Network {
    struct message_string {
        uint32_t length;
        char *content;
    };

    enum network_message_type {
        MSG_LIST_FILES = 1,
        MSG_UPLOAD = 2,
        MSG_DOWNLOAD = 3,
        MSG_DELETE = 4,
        MSG_EXIT = 5,
    };

    struct message_header {
        uint8_t protocol_version;
        uint8_t message_type;
        uint8_t username_length;
        uint8_t username_content[256];
    };

    struct message_upload {
        uint64_t timestamp;
        uint16_t filename_length;
        uint8_t filename[1024];
    };

    struct message_download {
        uint16_t filename_length;
        uint8_t filename[1024];
    };

    struct message_delete {
        uint64_t timestamp;
        uint16_t filename_length;
        uint8_t filename[1024];
    };

    struct message_exit {
        uint64_t timestamp;
        uint16_t filename_length;
        uint8_t filename[1024];
    };

    void init();
    int get_writer(int _sockfd);
    void flush_writer(int writer_id);
    void release_writer(int writer_id);
    void write(int writer_id, uint8_t *buf, uint64_t len);
    void write(int writer_id, uint8_t value);
    void write(int writer_id, uint16_t value);
    void write(int writer_id, uint32_t value);
    void write(int writer_id, message_string value);
    void write(int writer_id, struct message_header value);
    void write(int writer_id, struct message_download value);
    void write(int writer_id, struct message_list_files value);
    void write(int writer_id, struct message_upload value);
    void write(int writer_id, struct message_delete value);
    void write(int writer_id, struct message_exit value);

    namespace __internal {
        ssize_t send_message(int sockfd, uint32_t value);
        ssize_t send_message(int sockfd, uint16_t value);
        ssize_t send_message(int sockfd, uint8_t value);
        ssize_t send_message(int sockfd, struct message_string value);
        ssize_t send_message(int sockfd, struct message_header value);
        ssize_t send_message(int sockfd, struct message_download value);
    }
}

