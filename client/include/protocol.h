#ifndef PROTOCOL
#define PROTOCOL

#include "../include/connection.h"
#include "../include/netfs.h"

#include <cstdint>
#include <vector>

#define PROTOCOL_VERSION 1
#define PACKET_TYPE_HANDSHAKE 0
#define PACKET_TYPE_DOWNLOAD 1
#define PACKET_TYPE_UPLOAD 2
#define PACKET_TYPE_DELETE 3
#define PACKET_TYPE_LIST_FILES 4
#define PACKET_TYPE_EXIT 5
#define PACKET_TYPE_UPDATE 6

#define STATUS_SUCCESS 10
#define STATUS_EMPTY 11
#define STATUS_FAIL 20

typedef enum {
    event_file_modified = 1,
    event_file_deleted = 2,
} file_event_type_t;

typedef struct {
    file_event_type_t type;
    std::string filename;
} file_event_t;

bool request_handshake(connection_t *connection, std::string username, int *out_status);

bool request_download(connection_t *connection, int *out_status, uint8_t **out_buf, uint64_t *out_length);

bool request_upload(connection_t *connection, int *out_status);

bool request_delete(connection_t *connection, int *out_status);

bool request_list_files(connection_t *connection, int *out_status, std::vector<netfs::file_description_t> *out_files);

bool request_exit(connection_t *connection);

bool request_update(connection_t *connection, int *out_status, file_event_t *out_event);

#endif // !PROTOCOL
