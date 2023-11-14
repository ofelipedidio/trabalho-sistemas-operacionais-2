#include <iostream>

#define do_(e, f) if (!(e)) {std::cerr << "TEST FAIL: Failed to assert `" #e "` on file " f ", line " << __LINE__ << std::endl; return 1;}
#define assert(e) if (!(e)) {std::cerr << "TEST FAIL: Failed to assert `" #e "` on file " __FILE__ ", line " << __LINE__ << std::endl; return 1;} 

int main() { 
    assert(false);
    return 0; 
}

#define main __main
