#include <cstdint>
#include <string>
#include <vector>
#include <semaphore.h>
#include <pthread.h>
#include "file_manager.h"
#include "network.h"
#include "async_queue.h"

namespace App {
    void init(std::string username);
    void notify_new_file(std::string filename);
    void notify_modified(std::string filename);
    void notify_deleted(std::string filename);
    
    void task_done(uint8_t* mem);

    std::vector<FileManager::file_description> list_server();
    void upload_file(std::string path);
    void download_file(std::string filename);
    void delete_file(std::string path);
    void get_sync_dir(std::string username);
}

