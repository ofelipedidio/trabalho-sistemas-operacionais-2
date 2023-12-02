#include <cstdint>
#include <string>
#include <vector>
#include <semaphore.h>
#include <pthread.h>
#include "file_manager.h"
#include "network.h"
#include "async_queue.h"

namespace App {

    enum task_type {
        TASK_LIST_SERVER = 1,
        TASK_UPLOAD = 2,
        TASK_DOWNLOAD = 3,
        TASK_DELETE = 4,
        TASK_GET_SYNC = 5
    };

    typedef struct app_task {
        int type;
        int task_id;
        std::string username;
        std::string filename;
        uint8_t *content;
        std::vector<FileManager::file_description> files;

    } app_task;

    AsyncQueue::async_queue<app_task> task_queue;
    sem_t sync_cli;

    void init(std::string username);
    void notify_new_file(std::string filename);
    void notify_modified(std::string filename);
    void notify_deleted(std::string filename);
    

    void taks_done(uint8_t* mem);

    std::vector<file_description> list_server();
    void upload_file(std::string path);
    void download_file(std::string filename);
    void delete_file(std::string path);
    void get_sync_dir(std::string username);


}

