#include <cstdint>
#include <queue>
#include <string>
#include <vector>

#include "../include/filesystem.h"

namespace Network {
    enum task_type {
        TASK_LIST_FILES = 1,
        TASK_UPLOAD = 2,
        TASK_DOWNLOAD = 3,
        TASK_DELETE = 4,
        TASK_EXIT = 5,
    };

    struct network_task {
        int type;
        int task_id;
        std::string username;
        std::string filename;
        uint8_t *content;
        std::vector<FileSystem::file_description> files;
    };

    void init(std::string ip, std::string port);

    int upload_file(std::string username, std::string path);
    int download_file(std::string username, std::string path);
    int delete_file(std::string username, std::string path);
    int client_exit(std::string username);
    int list_files(std::string username, std::string path);

    bool try_get_task(network_task *task);
    void get_task(network_task *task);
}
