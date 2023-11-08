#include <cstdint>
#include <pthread.h>
#include <queue>
#include <string>
#include <vector>

#include "../include/network.h"

namespace Network {
    std::queue<network_task> task_queue;
    std::queue<network_task> done_queue;
    int LAST_TASK_ID = 0;

    pthread_mutex_t task_mutex = PTHREAD_MUTEX_INITIALIZER;

    int next_task_id() {
        return LAST_TASK_ID++;
    }

    int upload_file(std::string username, std::string path) {
        uint8_t *content = FileSystem::read_file(path);
        int task_id = next_task_id();
        task_queue.push({
                TASK_UPLOAD, 
                task_id, 
                username,
                path,
                content,
                std::vector<FileSystem::file_description>(),
                });
        return task_id;
    }

    int download_file(std::string username, std::string path) {
        int task_id = next_task_id();
        task_queue.push({
                TASK_DOWNLOAD, 
                task_id, 
                username,
                path,
                nullptr,
                std::vector<FileSystem::file_description>(),
                });
        return task_id;
    }

    int delete_file(std::string username, std::string path) {
        int task_id = next_task_id();
        task_queue.push({
                TASK_DELETE, 
                task_id, 
                username,
                path,
                nullptr,
                std::vector<FileSystem::file_description>(),
                });
        return task_id;
    }

    int client_exit(std::string username) {
        int task_id = next_task_id();
        task_queue.push({
                TASK_EXIT, 
                task_id, 
                username,
                "",
                nullptr,
                std::vector<FileSystem::file_description>(),
                });
        return task_id;
    }

    int list_files(std::string username, std::string path) {
        int task_id = next_task_id();
        task_queue.push({
                TASK_LIST_FILES, 
                task_id, 
                username,
                "",
                nullptr,
                std::vector<FileSystem::file_description>(),
                });
        return task_id;
    }

    bool try_get_task(network_task *task) {
        pthread_mutex_lock(&task_mutex);
        if (done_queue.size() > 0) {
            network_task done_task = done_queue.front();
            done_queue.pop();
            task->type = done_task.type;
            task->task_id = done_task.task_id;
            task->username = done_task.username;
            task->filename = done_task.filename;
            task->content = done_task.content;
            task->files = done_task.files;
            pthread_mutex_unlock(&task_mutex);
            return true;
        } else {
            pthread_mutex_unlock(&task_mutex);
            return false;
        }
    }

    void get_task(network_task *task) {
        pthread_mutex_lock(&task_mutex);
        // TODO - Didio: use synchronization mechanisms to avoid busy wait
        while (done_queue.size() <= 0) {
            pthread_mutex_unlock(&task_mutex);
            pthread_mutex_lock(&task_mutex);
        }

        network_task done_task = done_queue.front();
        done_queue.pop();
        task->type = done_task.type;
        task->task_id = done_task.task_id;
        task->username = done_task.username;
        task->filename = done_task.filename;
        task->content = done_task.content;
        task->files = done_task.files;
        pthread_mutex_unlock(&task_mutex);
    }
}

