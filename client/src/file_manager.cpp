#include <sys/types.h>
#include <fstream>
#include <cstdio>
#include <filesystem>
#include <chrono>
#include <iostream>
#include <sys/stat.h>
#include <semaphore.h>

#include "../include/file_manager.h"

namespace FileManager{

    bool write_file(std::string path, uint8_t *buf){ 
        std::ofstream file;
        // Felipe K - inotify doesnt send events while write is active
        sem_wait(&FSNotify::enable_notify);
        file.open(path, std::ios::out | std::ios::trunc);
        if(file.is_open()){
            file << buf;
            file.close();
            sem_post(&FSNotify::enable_notify);
            return true;
        }
        sem_post(&FSNotify::enable_notify);
        return false;
    }

    File_Metadata *read_file(std::string path){
        std::ifstream file(path, std::ios::in | std::ios::binary | std::ios::ate);
        File_Metadata* metadados = (File_Metadata*) malloc(sizeof(File_Metadata));

        if (!file.is_open()) 
        {
            (*metadados).mtime = 0;
            (*metadados).atime = 0;
            (*metadados).ctime = 0;
            (*metadados).length = 0;
            return metadados;
        }
        std::streampos size = file.tellg();
        
        struct stat mac;
        stat(path.c_str(), &mac);
        (*metadados).mtime = (uint64_t) mac.st_mtime;
        (*metadados).atime = (uint64_t) mac.st_atime;
        (*metadados).ctime = (uint64_t) mac.st_ctime;
        (*metadados).length = size;

        (*metadados).contents = (u_int8_t*) malloc(size);
        file.seekg (0, std::ios::beg);
        file.read ((char *)(*metadados).contents, size);
        file.close();
        return metadados;
    }

    bool delete_file(std::string path){
        if (remove(path.c_str()) == 0)
        {
            return true;
        }
        return false;
    }

    std::vector<file_description> list_files(std::string path){
        std::vector<file_description> files_list;

        if (std::filesystem::is_directory(path))
        {
            for (const auto& entry : std::filesystem::directory_iterator(path))
            {
                if(std::filesystem::is_regular_file(entry)){
                    struct stat result;
                    std::string filename = entry.path().string();
                    if (stat(filename.c_str(), &result)==0)
                    {
                        std::string::size_type i = filename.find(path);
                        if (i != std::string::npos)
                            filename.erase(i, path.length()+1);
                        files_list.emplace_back(filename, result.st_mtim.tv_sec, result.st_atim.tv_sec, result.st_ctim.tv_sec);
                    } else {
                        exit(69);
                    }
                }
            }

        }        

        return files_list;
    }
}
