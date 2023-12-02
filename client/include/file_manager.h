#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "../include/fs_notify.h"

namespace FileManager {
    /*
     * 'struct' used to list the files in a folder
     */
    typedef std::pair<std::string, uint64_t> file_description;

    /*
     * struct with the necessary information to upload a file
     */
    typedef struct Read_File_Metadata {
        long long int length;
        long long int mac;
        u_int8_t* contents;
    } File_Metadata;
    
    /*
     * writes the contents of buf to the file path
     */
    bool write_file(std::string path, uint8_t *buf);

    /*
     * reads the contents of the file in path and returns it in a File_Metadata struct
     * return a struct with values set to 0 in case of error
     */
    File_Metadata *read_file(std::string path);

    /*
     * deletes the file in path
     */
    bool delete_file(std::string path);

    /*
     * lists the files in the folder path 
     * returns an emprty vector in case of error
     */
    std::vector<file_description> list_files(std::string path);
}
