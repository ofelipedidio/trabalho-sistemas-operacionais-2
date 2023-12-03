#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "../include/fs_notify.h"

namespace FileManager {
    typedef std::tuple<std::string, uint64_t, uint64_t, uint64_t> file_description;

    typedef struct Read_File_Metadata {
        long long int length;
        uint64_t mtime;
        uint64_t atime;
        uint64_t ctime;
        u_int8_t* contents;
    } File_Metadata;
    
    bool write_file(std::string path, uint8_t *buf);
    File_Metadata *read_file(std::string path);
    bool delete_file(std::string path);
    std::vector<file_description> list_files(std::string path);
}
