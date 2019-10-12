#if ! defined(YALR_CLIOPTIONS_HPP)
#define YALR_CLIOPTIONS_HPP

#include <string>

struct CLIOptions {
    std::string output_file;
    std::string translate;
    std::string state_file;
    std::string input_file;
    bool debug = false;
    bool help = false;
};

#endif
