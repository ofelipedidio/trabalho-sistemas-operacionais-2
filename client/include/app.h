#include <cstdint>
#include <string>
#include <vector>
#include "file_manager.h"

namespace App {

    enum task_type {
        TASK_LIST_FILES = 1,
        TASK_UPLOAD = 2,
        TASK_DOWNLOAD = 3,
        TASK_DELETE = 4,
        TASK_EXIT = 5,
    };

    typedef struct app_task {
        int type;
        int task_id;
        std::string username;
        std::string filename;
        uint8_t *content;
        std::vector<FileManager::file_description> files;

    } app_task;

    void init(std::string username);
    void notify_new_file(std::string filename);
    void notify_modified(std::string filename);
    void notify_deleted(std::string filename);
    

    void taks_done(uint8_t* mem);

    std::vector<file_description> list_server();


}

