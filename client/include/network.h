#include <cstdint>
#include <queue>
#include <string>
#include <vector>

#include "../include/filesystem.h"

namespace Network {
    const int TASK_LIST_FILES = 1;
    const int TASK_UPLOAD = 2;
    const int TASK_DOWNLOAD = 3;
    const int TASK_DELETE = 4;
    const int TASK_EXIT = 5;

    int LAST_TASK_ID = 0;

    struct network_task {
        int type;
        int task_id;
        std::string username;
        std::string filename;
        uint8_t *content;
        std::vector<FileSystem::file_description> files;
    };

    std::queue<network_task> task_queue;
    std::queue<network_task> done_queue;

    void upload_file(std::string username, std::string path) {
        uint8_t *content = FileSystem::read_file(path);
        task_queue.push({
                TASK_UPLOAD, 
                LAST_TASK_ID++, 
                username,
                path,
                content,
                std::vector<FileSystem::file_description>(),
                });
    }

    void download_file(std::string username, std::string path);
    void delete_file(std::string username, std::string path);
    void client_exit(std::string username, std::string path);
    void list_files(std::string username, std::string path);

    bool try_get(network_task *task);
}
