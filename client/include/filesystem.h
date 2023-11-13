#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace FileManager {
    typedef std::pair<std::string, uint64_t> file_description;

    bool write_file(std::string path, uint8_t *buf);
    uint8_t *read_file(std::string path);
    bool delete_file(std::string path);
    std::vector<file_description> list_files(std::string path);
}

