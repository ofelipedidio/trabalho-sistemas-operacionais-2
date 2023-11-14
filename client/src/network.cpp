#include <cstdint>
#include <pthread.h>
#include <queue>
#include <string>
#include <vector>
#include <semaphore.h>

#include "../include/network.h"

namespace Network {
    pthread_t network_thread;
    std::string ip;
    std::string port;

    namespace __internal {
        /*
         * Queue
         */
        std::queue<network_task> task_queue;
        std::queue<network_task> done_queue;

        /*
         * Semaphores for the management of tasks.
         * - mutex: makes sure only one thread is using the task queue
         * - done_mutex: makes sure only one thread is using the done queue
         * - available: counts how many tasks are available to be done
         * - done: counts how many tasks are done.
         */
        sem_t mutex, done_mutex, available, done;

        /*
         * Generates a unique task ID.
         */
        int LAST_TASK_ID = 0;
        int next_task_id() {
            return __internal::LAST_TASK_ID++;
        }

        /*
         * Takes a task from que task queue and puts it into the 'task' parameter.
         * If the queue is empty, the thread is locked until a task is inserted.
         */
        void next_task(network_task *task) {
            // Wait until there is a task on the queue
            sem_wait(&available);
            // Wait until the queue can be safely accessed
            sem_wait(&mutex);

            // Get next task from queue
            network_task next = task_queue.front();
            task_queue.pop();
            
            // Release queue
            sem_post(&mutex);

            // Copy next's data onto task
            task->type = next.type;
            task->task_id = next.task_id;
            task->username = next.username;
            task->filename = next.filename;
            task->content = next.content;
            task->files = next.files;
        }

        /*
         * Adds the task to the done queue
         */
        void finish_task(network_task *task) {
            sem_wait(&done_mutex);
            done_queue.push(*task);
            sem_post(&done_mutex);
            sem_post(&done);
        }

        /*
         * The networking thread function
         */
        void *thread_function(void* p) {
            network_task task;

            while (1) {
                next_task(&task);
                // TODO - Didio: Process the task
                switch (task.type) {
                    case TASK_UPLOAD:
                        break;
                    case TASK_DOWNLOAD:
                        break;
                    case TASK_DELETE:
                        break;
                    case TASK_LIST_FILES:
                        break;
                    case TASK_EXIT:
                        break;
                }
                finish_task(&task);
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

        sem_init(&__internal::mutex, 0, 1);
        sem_init(&__internal::done_mutex, 0, 1);
        sem_init(&__internal::available, 0, 0);
        sem_init(&__internal::done, 0, 0);
         
        pthread_create(&network_thread, NULL, __internal::thread_function, nullptr);
    }

    /*
     * Adds an upload task to the task queue and returns the task id
     */
    int upload_file(std::string username, std::string path) {
        sem_wait(&__internal::mutex);
        int task_id = __internal::next_task_id();
        __internal::task_queue.push({

                TASK_UPLOAD, 
                task_id, 
                username,
                path,
                nullptr,
                std::vector<FileManager::file_description>(),
                });
        sem_post(&__internal::mutex);
        sem_post(&__internal::available);
        return task_id;
    }

    /*
     * Adds a download task to the task queue and returns the task id
     */
    int download_file(std::string username, std::string path) {
        sem_wait(&__internal::mutex);
        int task_id = __internal::next_task_id();
        __internal::task_queue.push({
                TASK_DOWNLOAD, 
                task_id, 
                username,
                path,
                nullptr,
                std::vector<FileManager::file_description>(),
                });
        sem_post(&__internal::mutex);
        sem_post(&__internal::available);
        return task_id;
    }

    /*
     * Adds a delete task to the task queue and returns the task id
     */
    int delete_file(std::string username, std::string path) {
        sem_wait(&__internal::mutex);
        int task_id = __internal::next_task_id();
        __internal::task_queue.push({
                TASK_DELETE, 
                task_id, 
                username,
                path,
                nullptr,
                std::vector<FileManager::file_description>(),
                });
        sem_post(&__internal::mutex);
        sem_post(&__internal::available);
        return task_id;
    }

    /*
     * Adds a exit task to the task queue and locks the thread until the networking subsystem is properly shutdown
     */
    int client_exit(std::string username) {
        sem_wait(&__internal::mutex);
        int task_id = __internal::next_task_id();
        __internal::task_queue.push({
                TASK_EXIT, 
                task_id, 
                username,
                "",
                nullptr,
                std::vector<FileManager::file_description>(),
                });
        sem_post(&__internal::mutex);
        sem_post(&__internal::available);
        return task_id;
    }

    /*
     * Adds a list files task to the task queue and returns the task id
     */
    int list_files(std::string username, std::string path) {
        sem_wait(&__internal::mutex);
        int task_id = __internal::next_task_id();
        __internal::task_queue.push({
                TASK_LIST_FILES, 
                task_id, 
                username,
                "",
                nullptr,
                std::vector<FileManager::file_description>(),
                });
        sem_post(&__internal::mutex);
        sem_post(&__internal::available);
        return task_id;
    }

    /*
     * Attempts to get a done task from the done queue. 
     * Returns true, if there is a task in the done queue. Returns false otherwise.
     */
    bool try_get_task(network_task *task) {
        // Try to acquire the `done` semaphore
        // (meaning that there is a task in the done queue)
        if (sem_trywait(&__internal::done) == 0) {
            // If it can acquire the `done`, it locks the `done_mutex` lock
            sem_wait(&__internal::done_mutex);

            // Pop an item out of the done_queue
            network_task done_task = __internal::done_queue.front();
            __internal::done_queue.pop();
            
            // Release the `done_mutex` lock
            sem_post(&__internal::done_mutex);

            // Copy the data from the done task to task
            task->type = done_task.type;
            task->task_id = done_task.task_id;
            task->username = done_task.username;
            task->filename = done_task.filename;
            task->content = done_task.content;
            task->files = done_task.files;

            // Signal that the opeartion of successful
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
        // Wait until there is a task in the done queue
        sem_wait(&__internal::done);
        // If it can acquire the `done`, it locks the `done_mutex` lock
        sem_wait(&__internal::done_mutex);

        // Pop an item out of the done_queue
        network_task done_task = __internal::done_queue.front();
        __internal::done_queue.pop();

        // Release the `done_mutex` lock
        sem_post(&__internal::done_mutex);

        // Copy the data from the done task to task
        task->type = done_task.type;
        task->task_id = done_task.task_id;
        task->username = done_task.username;
        task->filename = done_task.filename;
        task->content = done_task.content;
        task->files = done_task.files;
    }
}

