#include <cstdint>
#include <string>

typedef struct Program {

} Program;

Program init_program() {
}

namespace App {
    std::string username;

    void init(std::string username);

    void notify_new_file(std::string filename);
    void notify_modified(std::string filename);
    void notify_deleted(std::string filename);

    void taks_done(uint8_t* mem);
}

