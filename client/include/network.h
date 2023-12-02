#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "../include/netfs.h"

namespace Network {
    typedef enum {
        TASK_LIST_FILES = 1,
        TASK_UPLOAD = 2,
        TASK_DOWNLOAD = 3,
        TASK_DELETE = 4,
        TASK_EXIT = 5,
    } task_type_t;

    typedef struct {
        int type;
        int task_id;
        std::string username;
        std::string filename;
        uint64_t content_length;
        uint8_t *content;
        std::vector<netfs::file_description_t> files;
    } network_task_t;

    bool init(std::string ip, uint16_t port);

    int upload_file(std::string username, std::string path);
    int download_file(std::string username, std::string path);
    int delete_file(std::string username, std::string path);
    int client_exit(std::string username);
    int list_files(std::string username);

    bool try_get_task(network_task_t *task);
    void get_task(network_task_t *task);
    
    bool try_get_task_by_id(int task_id, network_task_t *task);
    void get_task_by_id(int task_id, network_task_t *task);
}
