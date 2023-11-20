#include <iostream>

#include "../include/net_protocol.h"

#define do_(e, f) if (!(e)) {std::cerr << "TEST FAIL: Failed to assert `" #e "` on file " f ", line " << __LINE__ << std::endl; return 1;}
#define assert(e) if (!(e)) {std::cerr << "TEST FAIL: Failed to assert `" #e "` on file " __FILE__ ", line " << __LINE__ << std::endl; return 1;} 

using namespace Network;

int main() { 
    char username[] = "didio";
    char filename[] = "file.txt";

    struct message_download message;
    message.header.protocol_version = 1;
    message.header.message_type = MSG_DOWNLOAD;
    message.header.username.length = 5;
    message.header.username.content = username;
    message.header.payload_length = 12;
    message.filename.length = 8;
    message.filename.content = filename;

    uint64_t message_size = compute_message_size(message);
    uint64_t offset = 0;
    uint8_t *buffer = new uint8_t[message_size];
    write_message(message, buffer, &offset);
    log_debug("Message written (download). The final offset is expected to be the same as the computed length. (" << log_value(offset) << ") (" << log_value(message_size) << ")");

    log_debug("header.protocol_version | " << GET_U16(buffer, 0));
    log_debug("header.message_type     | " << GET_U16(buffer, 2));
    log_debug("header.username.length  | " << GET_U32(buffer, 4));
    log_debug("header.username.cont[0] | " << GET_CHR(buffer, 8));
    log_debug("header.username.cont[1] | " << GET_CHR(buffer, 9));
    log_debug("header.username.cont[2] | " << GET_CHR(buffer, 10));
    log_debug("header.username.cont[3] | " << GET_CHR(buffer, 11));
    log_debug("header.username.cont[4] | " << GET_CHR(buffer, 12));
    log_debug("header.payload_length   | " << GET_U32(buffer, 13));
    log_debug("filename.length         | " << GET_U32(buffer, 17));
    log_debug("filename.content[0]     | " << GET_CHR(buffer, 21));
    log_debug("filename.content[1]     | " << GET_CHR(buffer, 22));
    log_debug("filename.content[2]     | " << GET_CHR(buffer, 23));
    log_debug("filename.content[3]     | " << GET_CHR(buffer, 24));
    log_debug("filename.content[4]     | " << GET_CHR(buffer, 25));
    log_debug("filename.content[5]     | " << GET_CHR(buffer, 26));
    log_debug("filename.content[6]     | " << GET_CHR(buffer, 27));
    log_debug("filename.content[7]     | " << GET_CHR(buffer, 28));

    return 0; 
}

#define main __main
