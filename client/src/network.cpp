#include <cstdint>
#include <optional>
#include <pthread.h>
#include <queue>
#include <string>
#include <vector>
#include <semaphore.h>

#include "../include/task_queue.h"
#include "../include/network.h"

namespace Network {
    pthread_t network_thread;
    std::string ip;
    std::string port;

    namespace __internal {
        TaskQueue::async_task_queue<Network::network_task> task_queue;

        /*
         * The networking thread function
         */
        void *thread_function(void* p) {
            network_task task;

            while (1) {
                task = task_queue.take();
                
                // TODO - Didio: Process the task
                switch (task.type) {
                    case TASK_LIST_FILES:
                        break;
                    case TASK_UPLOAD:
                        break;
                    case TASK_DOWNLOAD:
                        break;
                    case TASK_DELETE:
                        break;
                    case TASK_EXIT:
                        break;
                }
                task_queue.insert_done(task);
            }

            pthread_exit(nullptr);
        }
    }

    /*
     * Initializes the network subsystem and starts the network thread
     */
    void init(std::string ip, std::string port) {
        Network::ip = ip;
        Network::port = port;

        __internal::task_queue = TaskQueue::async_task_queue<network_task>();

        pthread_create(&network_thread, NULL, __internal::thread_function, nullptr);
    }

    /*
     * Adds an upload task to the task queue and returns the task id
     */
    int upload_file(std::string username, std::string path) {
        int task_id = __internal::task_queue.next_id();
        __internal::task_queue.insert({
                TASK_UPLOAD, 
                task_id, 
                username,
                path,
                nullptr,
                std::vector<FileManager::file_description>(),
                });
        return task_id;
    }

    /*
     * Adds a download task to the task queue and returns the task id
     */
    int download_file(std::string username, std::string path) {
        int task_id = __internal::task_queue.next_id();
        __internal::task_queue.insert({
                TASK_DOWNLOAD, 
                task_id, 
                username,
                path,
                nullptr,
                std::vector<FileManager::file_description>(),
                });
        return task_id;
    }

    /*
     * Adds a delete task to the task queue and returns the task id
     */
    int delete_file(std::string username, std::string path) {
        int task_id = __internal::task_queue.next_id();
        __internal::task_queue.insert({
                TASK_DELETE, 
                task_id, 
                username,
                path,
                nullptr,
                std::vector<FileManager::file_description>(),
                });
        return task_id;
    }

    /*
     * Adds a exit task to the task queue and locks the thread until the networking subsystem is properly shutdown
     */
    int client_exit(std::string username) {
        int task_id = __internal::task_queue.next_id();
        __internal::task_queue.insert({
                TASK_EXIT, 
                task_id, 
                username,
                "",
                nullptr,
                std::vector<FileManager::file_description>(),
                });
        return task_id;
    }

    /*
     * Adds a list files task to the task queue and returns the task id
     */
    int list_files(std::string username, std::string path) {
        int task_id = __internal::task_queue.next_id();
        __internal::task_queue.insert({
                TASK_LIST_FILES, 
                task_id, 
                username,
                "",
                nullptr,
                std::vector<FileManager::file_description>(),
                });
        return task_id;
    }

    /*
     * Attempts to get a done task from the done queue. 
     * Returns true, if there is a task in the done queue. Returns false otherwise.
     */
    bool try_get_task(network_task *task) {
        std::optional<network_task> _done_task = __internal::task_queue.try_take_done();
        if (_done_task) {
            network_task done_task = _done_task.value();
            task->type = done_task.type;
            task->task_id = done_task.task_id;
            task->username = done_task.username;
            task->filename = done_task.filename;
            task->content = done_task.content;
            task->files = done_task.files;
            return true;
        } else {
            return false;
        }
    }

    /*
     * Attempts to get a done task from the done queue.
     * Immediately returns, if there is a task in the done queue, waits otherwise.
     */
    void get_task(network_task *task) {
        network_task done_task = __internal::task_queue.take_done();

        // Copy the data from the done task to task
        task->type = done_task.type;
        task->task_id = done_task.task_id;
        task->username = done_task.username;
        task->filename = done_task.filename;
        task->content = done_task.content;
        task->files = done_task.files;
    }
}

