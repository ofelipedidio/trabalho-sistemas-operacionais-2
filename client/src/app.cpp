#include "../include/app.h"
#include <unordered_map>
#include <functional>
#include <fstream>
#include <iostream>
#include <semaphore.h>
#include <filesystem>

namespace App
{
    std::string username;
    std::string path;
    std::unordered_map<std::string, size_t> dir_hash;
    sem_t mutex;

    size_t hash_string(const std::string &str)
    {
        std::hash<std::string> hasher;
        return hasher(str);
    }

    size_t hash_file(const std::string &filename)
    {
        std::ifstream file(path + "/" + filename, std::ios::binary);
        if (!file.is_open())
        {
            std::cerr << "Error opening file: " << filename << std::endl;
            return 0;
        }

        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

        return hash_string(content);
    }

    void init(std::string username)
    {
        App::username = username;
        path = "sync_dir_" + username;
        sem_init(&mutex, 0, 1);
    }

    void notify_new_file(std::string filename)
    {
        sem_wait(&mutex);
        if (dir_hash.count(filename) == 0 || (dir_hash[filename] != hash_file(filename)))
        {
            std::string path_file = path + "/" + filename;
            dir_hash[filename] = hash_file(filename);
            Network::upload_file(username, path_file);
        }
        sem_post(&mutex);
    }

    void notify_modified(std::string filename)
    {
        sem_wait(&mutex);
        if (dir_hash.count(filename) == 0 || (dir_hash[filename] != hash_file(filename)))
        {
            std::string path_file = path + "/" + filename;
            dir_hash[filename] = hash_file(filename);
            Network::upload_file(username, path_file);
        }
        sem_post(&mutex);
    }

    void notify_deleted(std::string filename)
    {
        sem_wait(&mutex);
        if (dir_hash.count(filename) != 0)
        {
            dir_hash.erase(filename);
            Network::delete_file(username, filename);
        }
        sem_post(&mutex);
    }

    void network_new_file(std::string filename, uint8_t *buf)
    {
        sem_wait(&mutex);

        if (dir_hash.count(filename) == 0 || (dir_hash[filename] != hash_file(filename)))
        {
            std::string path_file = path + "/" + filename;
            dir_hash[filename] = hash_file(filename);
            if (!FileManager::write_file(path_file, buf))
            {
                std::cout << "Failed to create file from server: " << filename << std::endl;
            }
        }
        sem_post(&mutex);
    }

    void network_modified(std::string filename, uint8_t *buf)
    {
        sem_wait(&mutex);

        if (dir_hash.count(filename) == 0 || (dir_hash[filename] != hash_file(filename)))
        {
            std::string path_file = path + "/" + filename;
            dir_hash[filename] = hash_file(filename);
            if (!FileManager::write_file(path_file, buf))
            {
                std::cout << "Failed to create file from server: " << filename << std::endl;
            }
        }
        sem_post(&mutex);
    }

    void network_deleted(std::string filename)
    {
        sem_wait(&mutex);
        if (dir_hash.count(filename) != 0)
        {
            dir_hash.erase(filename);
            FileManager::delete_file(path + "/" + filename);
        }
        sem_post(&mutex);
    }

    std::vector<FileManager::file_description> list_server()
    {
        sem_wait(&mutex);
        Network::network_task task;
        int task_id = Network::list_files(username);
        Network::get_task_by_id(task_id, &task);
        sem_post(&mutex);
        return task.files;
    }

    void upload_file(std::string path)
    {
        sem_wait(&mutex);
        Network::network_task task;
        std::string filename = std::filesystem::path(path).filename().string();
        Network::upload_file(username, filename);
        sem_post(&mutex);
    }

    void download_file(std::string filename)
    {
        sem_wait(&mutex);
        // TODO
        sem_post(&mutex);
    }

    void delete_file(std::string filename)
    {
        sem_wait(&mutex);
        Network::network_task task;
        Network::delete_file(username, filename);        
        sem_post(&mutex);
    }
}
