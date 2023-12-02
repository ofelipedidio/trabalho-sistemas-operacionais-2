#include <sys/inotify.h>
#include<semaphore.h>
#include <string>

namespace FSNotify {
    #define BUFFER_SIZE 8192
    
    #ifndef GLOBAL_H
    #define GLOBAL_H

    inline sem_t enable_notify;

    #endif

    namespace __internal{
        /*
         * resposible for asynchronously identifying changes in sync_dir_ 
         */
        void *thread_function(void *arg);
    }
    /*
     * notifies the app that an modification event has happened
     */
    bool modified(inotify_event *event);
    /*
     * notifies the app that an creation event has happened
     */
    bool created(inotify_event *event);
    /*
     * notifies the app that an deletion event has happened
     */
    bool deleted(inotify_event *event);
    /*
     * initializes the inotify thread in the folder path
     */
    void init(std::string path);
}
