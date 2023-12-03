#ifndef PROTOCOL
#define PROTOCOL

#include <cstdint>

#include "../include/connection.h"
#include "../include/client.h"
#include "../include/file_manager.h"

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

typedef struct {
    uint16_t protocol_version;
    uint8_t packet_type;
} packet_header_t;

bool read_header(connection_t *connection, packet_header_t *header);

bool handshake(connection_t *connection, std::string *username);

bool receive_packet(connection_t *connection, packet_header_t *header, std::string *filename, uint64_t *length, uint8_t **bytes);

bool respond_handshake_success(connection_t *connection);

bool respond_handshake_fail(connection_t *connection);

bool respond_download_success(connection_t *connection, uint8_t *buf, uint64_t length);

bool respond_download_fail(connection_t *connection);

bool respond_upload_success(connection_t *connection);

bool respond_upload_fail(connection_t *connection);

bool respond_delete_success(connection_t *connection);

bool respond_delete_fail(connection_t *connection);

bool respond_list_files_success(connection_t *connection, std::vector<netfs::file_description_t> files);

bool respond_list_files_fail(connection_t *connection);

bool respond_update_some(connection_t *connection, file_event_t event);

bool respond_update_none(connection_t *connection);

#endif // !PROTOCOL
