#pragma once

#include <string>

struct cli_options {
    std::string output_file;
    std::string translate;
    std::string state_file;
    std::string input_file;
    std::string algorithm;
    bool debug = false;
};
