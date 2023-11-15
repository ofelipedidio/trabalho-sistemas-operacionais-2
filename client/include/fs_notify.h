#include <sys/inotify.h>
#include <string>

namespace FSNotify {
    #define BUFFER_SIZE 8192
    
    pthread_t FSNotify_thread;
    pthread_attr_t attr;
    sem_t enable_notify;

    namespace __internal{
        void *thread_function(void *arg);
    }
    bool modified(inotify_event *event);
    bool created(inotify_event *event);
    bool deleted(inotify_event *event);
    void init(std::string path);
}
