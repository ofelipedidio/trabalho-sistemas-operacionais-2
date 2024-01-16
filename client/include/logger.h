#ifndef LOGGER
#define LOGGER

#include <iostream>
#include <mutex>
#include <pthread.h>

inline std::string get_thread_name() {
    char name[1024];
    if (pthread_getname_np(pthread_self(), name, 1024) != 0) {
        return std::string(name);
    }
    return "failed_to_retrieve";
}

// Set log level
#define LOG_DEBUG
#define LOG_ERROR

inline std::mutex _log_mutex;

#ifdef LOG_DEBUG
#define log_debug(msg) std::cerr << "[DEBUG] [Thread " << get_thread_name() << "] [File \"" __FILE__ "\", line " << __LINE__ << "]" << msg << std::endl;
#else
#define log_debug(msg) 
#endif

#ifdef LOG_ERROR
#define log_error(msg) std::cerr << "[ERROR] [Thread " << get_thread_name() << "] [File \"" __FILE__ "\", line " << __LINE__ << "]" << msg << std::endl;
#else
#define log_error(msg)
#endif

#define log_value(x) #x " = `" << x << "`"

#endif
