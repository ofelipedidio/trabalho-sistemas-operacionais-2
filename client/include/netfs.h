#ifndef NETFS
#define NETFS

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace netfs {
    typedef struct {
        std::string filename;
        uint64_t mac;
    } file_description_t;

    typedef struct {
        uint64_t length;
        uint64_t mac;
        uint8_t* contents;
    } file_metadata_t;
    
    /*
     * Writes a file to disk. Returns true if the file is successfully written
     */
    bool write_file(std::string path, uint8_t *buf, uint64_t length);

    /*
     * Reads a file from disk. Returns true if the file is successfully read 
     * and store the file data on out_metadata.
     */
    bool read_file(std::string path, file_metadata_t *out_metadata);

    /*
     * Deletes a file from disk. Returns true if the file is successfully deleted.
     */
    bool delete_file(std::string path);

    /*
     * List files from disk.
     */
    bool list_files(std::string path, std::vector<file_description_t> *out_files);
}

#endif // !NETFS
