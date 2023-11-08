#include <fstream>
#include <cstdio>
#include <filesystem>
#include <chrono>


#include "../include/filesystem.h"


namespace FileSystem{

    bool write_file(std::string path, uint8_t *buf){
        std::ofstream file;
        file.open(path, std::ios::out | std::ios::trunc);// opens file for output and deletes what was already there
        if(file.is_open()){
            file << buf;
            file.close();
            return true;
        }
        return false;
    }

    uint8_t *read_file(std::string path){
        //TODO: Felipe K: garantee *buf stays allocated after functions ends
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
                    std::string filename = entry.path().filename().string();
                    uint64_t modified_time = std::filesystem::last_write_time(entry).time_since_epoch().count();
                    files_list.emplace_back(filename,modified_time);
                }
            }
            
        }        

        return files_list;
    }
}
