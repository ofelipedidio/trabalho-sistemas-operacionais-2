#pragma once

#include <string>
#include <vector>
#include <pthread.h>
#include "../include/file_manager.h"
#include "../include/network.h"
#include "../include/async_queue.h"
#include "../include/netfs.h"

namespace App {
    // Initialization
    void init(std::string username);

    // Notify -> app
    void notify_new_file(std::string filename);
    void notify_modified(std::string filename);
    void notify_deleted(std::string filename);

    // Network -> app
    void network_modified(std::string filename, uint8_t *buf, uint64_t length);
    void network_deleted(std::string filename);

    // Cli -> app
    void upload_file(std::string path);
    void download_file(std::string filename);
    void delete_file(std::string filename);
    std::vector<netfs::file_description_t> list_server();

    // Version control
    size_t hash_string(const std::string& str);
    size_t hash_file(const std::string& filename);

    // Utils
    std::string get_username();
}

