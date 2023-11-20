#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace FileManager {
    typedef std::pair<std::string, uint64_t> file_description;

    struct file_bytes {
        uint8_t *data;
        uint64_t length;
    };

    bool write_file(std::string path, uint8_t *buf);
    file_bytes read_file(std::string path);
    bool delete_file(std::string path);
    std::vector<file_description> list_files(std::string path);
}


