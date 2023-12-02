#include <atomic>
#include <cstdint>
#include <iostream>
#include <optional>
#include <pthread.h>
#include <queue>
#include <string>
#include <strings.h>
#include <vector>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "../include/async_queue.h"
#include "../include/network.h"

namespace Network {
    pthread_t network_thread;
    std::string ip;
    uint16_t port;
    std::atomic<int> last_task_id;
    int sockfd;

    namespace __internal {
        AsyncQueue::async_queue<Network::network_task> task_queue;
        AsyncQueue::async_queue<Network::network_task> done_queue;

        /*
         * The networking thread function
         */
        void *thread_function(void* p) {
            network_task task;

            while (true) {
                log_debug("Task queue size = " << task_queue.size());
                task = task_queue.pop();
                log_debug("(B) Task queue size = " << task_queue.size());
                
                // TODO - Didio: Process the task
                switch (task.type) {
                    case TASK_LIST_FILES:
                        break;
                    case TASK_UPLOAD:
                        break;
                    case TASK_DOWNLOAD:
                        {

                        }

                        break;
                    case TASK_DELETE:
                        break;
                    case TASK_EXIT:
                        break;
                }
                done_queue.push(task.task_id, task);
            }

            pthread_exit(nullptr);
        }
    }

    /*
     * Initializes the network subsystem and starts the network thread
     */
    bool init(std::string ip, uint16_t port) {
        Network::ip = ip;
        Network::port = port;

        __internal::task_queue = AsyncQueue::async_queue<network_task>();
        __internal::done_queue = AsyncQueue::async_queue<network_task>();

        /*
         * Create socket and connect to server
         */
        {
            struct sockaddr_in server_addr;
            server_addr.sin_family = AF_INET;
            server_addr.sin_port = htons(port);
            inet_aton(ip.c_str(), &server_addr.sin_addr);
            bzero(&server_addr.sin_zero, 8);

            sockfd = socket(AF_INET, SOCK_STREAM, 0);
            if (sockfd == -1) {
                // TODO - Didio: Handle error;
                return false;
            }

            int connect_response = connect(sockfd, (struct sockaddr *) (&server_addr), sizeof(struct sockaddr_in));
            if (connect_response < 0) {
                // TODO - Didio: Handle error
                return false;
            }
        }

        pthread_create(&network_thread, NULL, __internal::thread_function, NULL);
        return true;
    }

    /*
     * Adds an upload task to the task queue and returns the task id
     */
    int upload_file(std::string username, std::string path) {
        int task_id = last_task_id++;
        __internal::task_queue.push(task_id, {
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
        int task_id = last_task_id++;
        __internal::task_queue.push(task_id, {
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
        int task_id = last_task_id++;
        __internal::task_queue.push(task_id, {
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
        int task_id = last_task_id++;
        __internal::task_queue.push(task_id, {
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
        int task_id = last_task_id++;
        __internal::task_queue.push(task_id, {
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
        std::optional<network_task> _done_task = __internal::done_queue.try_pop();
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
        network_task done_task = __internal::done_queue.pop();

        // Copy the data from the done task to task
        task->type = done_task.type;
        task->task_id = done_task.task_id;
        task->username = done_task.username;
        task->filename = done_task.filename;
        task->content = done_task.content;
        task->files = done_task.files;
    }
    
    bool try_get_task_by_id(int task_id, network_task *task) {
        std::optional<network_task> _done_task = __internal::done_queue.try_pop_by_id(task_id);
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

    void get_task(int task_id, network_task *task) {
        network_task done_task = __internal::done_queue.pop_by_id(task_id);

        // Copy the data from the done task to task
        task->type = done_task.type;
        task->task_id = done_task.task_id;
        task->username = done_task.username;
        task->filename = done_task.filename;
        task->content = done_task.content;
        task->files = done_task.files;
    }
}

