#pragma once

#include <string>
#include "../include/tabulate.hpp" 
#include "../include/file_manager.h" 

namespace Cli{
    void init(const std::string& username);
    void upload_file(const std::string& username, const std::string& filepath);
    void download_file(const std::string& username, const std::string& filename);
    void delete_file(const std::string& username, const std::string& filename);
    void list_server_files(const std::string& username);
    void list_client_files(const std::string& username);
    void cli_exit(const std::string& username);
    tabulate::Table create_file_table(std::vector<FileManager::file_description> files);
}
