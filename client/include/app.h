#pragma once

#include <string>
#include <vector>
#include <pthread.h>
#include "file_manager.h"
#include "network.h"
#include "async_queue.h"

namespace App {
    void init(std::string username);
    void notify_new_file(std::string filename);
    void notify_modified(std::string filename);
    void notify_deleted(std::string filename);

    void network_new_file(std::string filename, uint8_t *buf);
    void network_modified(std::string filename, uint8_t *buf);
    void network_deleted(std::string filename);
    
    std::vector<FileManager::file_description> list_server();
    void upload_file(std::string path);
    void download_file(std::string filename);
    void delete_file(std::string filename);

    size_t hash_string(const std::string& str);
    size_t hash_file(const std::string& filename);
}

