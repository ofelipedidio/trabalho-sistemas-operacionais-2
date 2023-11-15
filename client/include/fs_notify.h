#include <sys/inotify.h>
#include<semaphore.h>
#include <string>

namespace FSNotify {
    #define BUFFER_SIZE 8192
    
    #ifndef GLOBAL_H
    #define GLOBAL_H

    inline sem_t enable_notify; // Felipe K - avoid multiple declarations of variable

    #endif

    


    namespace __internal{
        void *thread_function(void *arg);
    }
    bool modified(inotify_event *event);
    bool created(inotify_event *event);
    bool deleted(inotify_event *event);
    void init(std::string path);
}
