#include <iostream>
#include <vector>
#include <functional>
#include <map>
#include <utility>
#include <filesystem>

#include "../include/app.h"
#include "../include/cli.h"
#include "../include/network.h"
#include "../include/file_manager.h"


namespace Cli{

    std::string path;

    void init(const std::string& username){
        
        path = "sync_dir_" + username;

        using Command_Function = std::function<void(const std::string&)>;

        std::map<std::string, Command_Function> command_map = {
            {"upload", [&username](const std::string& ) {
                std::string filepath;
                std::cin >> filepath;
                upload_file(username, filepath);
            }},
            {"download", [&username](const std::string& ) {
                std::string filename;
                std::cin >> filename;
                download_file(username, filename);
            }},
            {"delete", [&username](const std::string&) {
                std::string filename;
                std::cin >> filename;
                delete_file(username, filename);
            }},
            {"list_server", [&username](const std::string&) {
                list_server_files(username);
            }},
            {"list_client", [&username](const std::string&) {
                list_client_files(username);
            }},
            {"exit", [&username](const std::string&) {
                cli_exit(username);
            }}
        };

        std::string user_input;
        while (true) {
            std::cout<<"> ";
            bool aux = (bool)(std::cin >> user_input);
            if(!aux)break;
            auto it = command_map.find(user_input);
            if (it != command_map.end()) {
                it->second(username);
            } else {
                std::cout << "Invalid\n";
            }
        }
    }


    void upload_file(const std::string& username, const std::string& filepath) {
        if(std::filesystem::exists(filepath)){
            App::upload_file(filepath);
            std::cout << "uploading file in " << filepath << std::endl;
        }else{
            std::cout << "file not found" << std::endl;
        }
    }

    void download_file(const std::string& username, const std::string& filename) {
        std::cout << "downloading " << filename << std::endl;
        App::download_file(filename);
    }

    void delete_file(const std::string& username, const std::string& filename) {
        if(std::filesystem::exists(path + "/" + filename)){
            App::delete_file(filename);
            std::cout << "deleting "<< filename << std::endl;
        }else{
            std::cout << "file not found" << std::endl;
        }
    }

    void list_server_files(const std::string& username) {
        std::vector<netfs::file_description_t> server_files = App::list_server();
        tabulate::Table files = create_file_table(server_files);
        std::cout << files << std::endl;
    }

    void list_client_files(const std::string& username) {
        std::vector<netfs::file_description_t> client_files;
        if (!netfs::list_files(path, &client_files)) {
            // TODO - Didio: Handle case when the files cannot be listed
        }
        tabulate::Table files = create_file_table(client_files);
        std::cout << files << std::endl;
    }

    void cli_exit(const std::string& username) {
        Network::client_exit(username);
        std::cout<<"exit with sucess"<<std::endl;
        exit(0);
    }

    tabulate::Table create_file_table(std::vector<netfs::file_description_t> files){
        tabulate::Table file_table;
        file_table.add_row({"filename", "mac"});

        for(auto [filename, mac]:files){
            file_table.add_row({filename, std::to_string(mac)});
        }

        for (size_t i = 0; i < 2; ++i) {
            file_table[0][i].format()
            .font_color(tabulate::Color::yellow)
            .font_align(tabulate::FontAlign::center)
            .font_style({tabulate::FontStyle::bold});
        }
        return file_table;
    }

}
