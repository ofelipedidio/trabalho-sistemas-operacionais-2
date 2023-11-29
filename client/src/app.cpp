#include "../include/app.h"

namespace App {
    std::string username;

    void *thread_function(void* p) {
            app_task task;

            while (true) {
                log_debug("Task queue size = " << task_queue.size());
                task = task_queue.pop();
                log_debug("(B) Task queue size = " << task_queue.size());
                
                switch (task.type) {
                    case TASK_LIST_SERVER:
                        
                        break;
                    case TASK_UPLOAD:
                        break;
                    case TASK_DOWNLOAD:
                        break;
                    case TASK_DELETE:
                        Network::delete_file(task.username, task.path); // probably worng
                        break;
                    case TASK_GET_SYNC:
                        break;
                }
                done_queue.push(task);
            }

            pthread_exit(nullptr);
        }



    void init(std::string username) {
        App::username = username;
        sem_init(&sync_cli, 0, 0);
        task_queue = AsyncQueue::async_queue<app_task>();

    }
}

