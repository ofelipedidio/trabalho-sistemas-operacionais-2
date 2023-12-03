#include <iostream>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/inotify.h>
#include <pthread.h>
#include <semaphore.h>


#include "../include/logger.h"
#include "../include/fs_notify.h"
#include "../include/app.h"


namespace FSNotify {
    pthread_t FSNotify_thread;
    pthread_attr_t attr;
    
    namespace __internal{
        void *thread_function(void *arg){
            std::string *username = static_cast<std::string*>(arg);
            std::string _path = "sync_dir_" + (*username);
            std::string *path = &_path;

            int Fd = inotify_init();
    
            if (Fd == -1) {
                std::cerr << "inotify_init error\n";
                exit(EXIT_FAILURE);
            }

            u_int32_t mask = IN_MODIFY | IN_CREATE | IN_DELETE | IN_MOVE | IN_CLOSE_WRITE; 

            int watchFd = inotify_add_watch(Fd, (*path).c_str(), mask);
            if (watchFd == -1) {
                std::cerr << "inotify_add_watch error\n";
                std::cerr << *path << std::endl;
                close(Fd);
                exit(EXIT_FAILURE);
            }

            char buffer[BUFFER_SIZE];
            ssize_t bytesRead;


            while (true)
            {
                bytesRead = read(Fd, buffer, sizeof(buffer));
                if (bytesRead == -1) {
                    std::cerr << "read error\n";
                    close(Fd);
                    exit(EXIT_FAILURE);
                }
                sem_wait(&enable_notify);  //espera o fim do write para enviar notificações
                for (char *p = buffer; p < buffer + bytesRead;) {
                    struct inotify_event *event = reinterpret_cast<struct inotify_event *>(p);

                    if ((event->mask & IN_MODIFY) | (event->mask & IN_CLOSE_WRITE)) {
                        if (!modified(event))
                        {
                            std::cerr << "failed to notify modification\n";
                        }
                    }

                    if ((event->mask & IN_CREATE) | (event->mask & IN_MOVED_TO)) {
                        if(!created(event)){
                            std::cerr << "failed to notify creation\n";
                        }
                    }

                    if ((event->mask & IN_DELETE) | (event->mask & IN_MOVED_FROM)) {
                        if(!deleted(event)){
                            std::cerr << "failed to notify deletion\n";
                        }
                    }

                    p += sizeof(struct inotify_event) + event->len;
                }
                sem_post(&enable_notify);
            }
            
            close(Fd);   
        }
    }


    bool modified(inotify_event *event){
        std::string name(event->name,event->len);
        App::notify_modified(name);
        return true;
    }
    bool created(inotify_event *event){
        std::string name(event->name,event->len);
        App::notify_new_file(name);
        return true;
    }
    bool deleted(inotify_event *event){
        std::string name(event->name,event->len);
        App::notify_deleted(name);
        return true;
    }


    void init(std::string username) {
        sem_init(&enable_notify, 0, 1);
        pthread_attr_init(&attr);

        pthread_create(&FSNotify_thread, &attr, __internal::thread_function, &username);
    } 
}
