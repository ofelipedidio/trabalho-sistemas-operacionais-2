#include <sys/types.h>
#include <fstream>
#include <cstdio>
#include <filesystem>
#include <chrono>
#include <iostream>
#include <sys/stat.h>


#include "../include/filesystem.h"


namespace FileManager{

    bool write_file(std::string path, uint8_t *buf){ //true em caso de sucesso
        std::ofstream file;
        //TODO: Felipe K - garantir que inotify não pegue oq esta sendo escrito como algo novo
        file.open(path, std::ios::out | std::ios::trunc);// opens file for output and deletes what was already there
        if(file.is_open()){
            file << buf;
            file.close();
            return true;
        }
        return false;
    }

    uint8_t *read_file(std::string path){
        std::ifstream file(path, std::ios::in | std::ios::binary | std::ios::ate);
        if (!file.is_open()) //retorna um ponteiro com 0 caso não consiga acessar o arquivo
        {
            u_int8_t *data = (u_int8_t*) malloc(sizeof(char));
            *data = '\0';
            return data;
        }
        std::streampos size = file.tellg();
        u_int8_t *data = (u_int8_t*) malloc(size);//possivelmente precise de uns bytes no inicio pra metadados
        file.seekg (0, std::ios::beg);
        file.read ((char*)data, size);
        file.close();
        return data;
    }

    bool delete_file(std::string path){//true em caso de sucesso 
        if (remove(path.c_str()) == 0)
        {
            return true;
        }
        return false;
    }

    std::vector<file_description> list_files(std::string path){//retorna um vetor vazio caso path não seja diretório
        std::vector<file_description> files_list;

        if (std::filesystem::is_directory(path))
        {
            for (const auto& entry : std::filesystem::directory_iterator(path))
            {
                if(std::filesystem::is_regular_file(entry)){//só retorna informações de arquivos que não sejam diretórios
                    struct stat result;
                    std::string filename = entry.path().string();
                    if (stat(filename.c_str(), &result)==0)
                    {
                        auto mod_time = result.st_mtime;
                        uint64_t modified_time = mod_time;
                        files_list.emplace_back(filename, modified_time);
                    } else {
                        exit(69);
                    }
                }
            }

        }        

        return files_list;
    }
}
