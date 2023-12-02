#include "../include/file_manager.h"

#include <cerrno>
#include <cstdint>
#include <sys/types.h>
#include <fstream>
#include <cstdio>
#include <filesystem>
#include <chrono>
#include <iostream>
#include <sys/stat.h>

namespace FileManager {
    bool write_file(std::string path, uint8_t *buf, uint64_t length) {
        // Open the file
        FILE *file = fopen(path.c_str(), "w");
        if (file == NULL) {
            std::cerr << "ERROR: [writing file `" << path << "`] fopen failed with errno = `" << errno << "`" << std::endl;
            return false;
        }

        // Write the contents of the buffer into the file
        size_t write_amount = 0;
        while (write_amount < length) {
            size_t write_size = fwrite(
                    buf + write_amount,
                    sizeof(uint8_t), 
                    length - write_amount,
                    file);
            if (write_size < 0) {
                std::cerr << "ERROR: [writing file `" << path << "`] fwrite failed with errno = `" << errno << "`" << std::endl;
                fclose(file);
                return false;
            }
            write_amount += write_size;
        }

        // Close the file
        fclose(file);

        return true;
    }

    bool read_file(std::string path, file_metadata_t *out_metadata) {
        // Open the file
        FILE *file = fopen(path.c_str(), "r");
        if (file == NULL) {
            std::cerr << "ERROR: [reading file `" << path << "`] fopen failed with errno = `" << errno << "`" << std::endl;
            return false;
        }

        // Get file length
        uint64_t length;
        fseek(file, 0, SEEK_END);
        length = ftell(file);
        fseek(file, 0, SEEK_SET); // Reset file to read from beginning

        // Allocate memory for the file contents
        uint8_t *bytes = (uint8_t*) malloc(length * sizeof(uint8_t));
        if (bytes == NULL) {
            std::cerr << "ERROR: [reading file `" << path << "`] malloc failed with errno = `" << errno << "`" << std::endl;
            fclose(file);
            return false;
        }

        // Read bytes from file into the output buffer
        size_t read_amount = 0;
        while (read_amount < length) {
            size_t read_size = fread(
                    bytes + read_amount,
                    sizeof(uint8_t),
                    length - read_amount,
                    file);
            if (read_size < 0) {
                std::cerr << "ERROR: [reading file `" << path << "`] fread with errno = `" << errno << "`" << std::endl;
                fclose(file);
                return false;
            }
            read_amount += read_size;
        }

        // Get last modified time of the file
        struct stat _stat;
        if (stat(path.c_str(), &_stat) != 0) {
            std::cerr << "ERROR: [reading file `" << path << "`] stat failed with errno = `" << errno << "`" << std::endl;
            return false;
        }
        uint64_t mac = _stat.st_mtime;

        // Close the file
        fclose(file);

        // Return
        *out_metadata = {length, mac, bytes};
        return true;
    }

    bool delete_file(std::string path) {
        if (remove(path.c_str()) != 0) {
            std::cerr << "ERROR: [deleting file `" << path << "`] remove failed with errno = `" << errno << "`" << std::endl;
            return false;
        }

        return true;
    }

    bool list_files(std::string path, std::vector<file_description_t> *out_files) {
        std::vector<file_description_t> files_list = std::vector<file_description_t>();

        if (!std::filesystem::is_directory(path)) {
            std::cout << "ERROR [listing files `" << path << "`] Path is not a directory" << std::endl;
            return false;
        }

        for (const auto& entry : std::filesystem::directory_iterator(path)) {
            if (std::filesystem::is_regular_file(entry)) {
                struct stat result;
                std::string filename = entry.path().string();
                if (stat(filename.c_str(), &result) == 0) {
                    auto mod_time = result.st_mtime;
                    uint64_t modified_time = mod_time;
                    files_list.push_back({filename, modified_time});
                } else {
                    std::cout << "ERROR [listing files `" << path << "`] Could not stat file `" << filename << "`" << std::endl;
                    return false;
                }
            }
        }

        *out_files = files_list;
        return true;
    }
}

