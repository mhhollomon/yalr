#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <filesystem>
#include <map>
#include <stdexcept>
#include <regex>

#include <cstdlib>
#include <cstring>


namespace fs = std::filesystem;

//
// Config file parsing
//
class ConfigFile {
    enum class state_t { sNormal, sBlock };
    std::string last_error = "";

    std::map<std::string, std::string>data;

  public :
    std::string error() const { return last_error; }

    bool read_stream(std::istream& strm) {

        last_error = "";
        std::string line;
        int line_count = 0;
        state_t state = state_t::sNormal;
        std::string current_block_key;
        std::string current_block_content;

        while (std::getline(strm, line)) {

            line_count += 1;


            auto lv = std::string_view(line);

            if (state == state_t::sNormal) {
                if (lv.size() < 3) continue;
                if (lv[0] == '#') continue;
                if (lv[0] != '.') {
                    last_error = "line " + std::to_string(line_count) + 
                        ": Unknown line prefix " + line.substr(0, 4) + "...";
                    return false;
                }

                auto index = lv.find(' ');
                if (index == std::string_view::npos) {
                    index = lv.size();
                }
                auto type = lv.substr(0, index);

                if (std::string_view(".entry").compare(0, type.size(), type) == 0) {
                    lv.remove_prefix(index);
                    index = lv.find_first_not_of(' ');
                    if (index == std::string_view::npos) {
                        last_error = "line " + std::to_string(line_count) +
                            ": No key could be found";
                        return false;
                    }

                    lv.remove_prefix(index);

                    auto key_end = lv.find(' ');
                    auto key = lv.substr(0, key_end);

                    lv.remove_prefix(key_end);

                    index = lv.find_first_not_of(' ');
                    if (index == std::string_view::npos) {
                        last_error = "line " + std::to_string(line_count) +
                            ": No value could be found";
                        return false;
                    }

                    lv.remove_prefix(index);

                    data.emplace(key, lv);

                    continue;

                } else if (std::string_view(".block").compare(0, type.size(), type) == 0) {
                    // its a block
                    lv.remove_prefix(index);
                    index = lv.find_first_not_of(' ');
                    if (index == std::string_view::npos) {
                        last_error = "line " + std::to_string(line_count) +
                            ": No key could be found";
                        return false;
                    }

                    lv.remove_prefix(index);

                    auto key_end = lv.find(' ');
                    auto key = lv.substr(0, key_end);

                    current_block_key = std::string(key);
                    current_block_content.clear();

                    state = state_t::sBlock;

                    std::cerr << "Found block for key '" << current_block_key << "'\n";

                    continue;
                } else {
                    last_error = "line " + std::to_string(line_count) + 
                        ": Unknown type '" + line.substr(0, type.size()) + "'";
                    return false;
                }
            } else if (state == state_t::sBlock) {
                auto blockend = std::string_view(".blockend");
                if (lv.compare(0, blockend.size(), blockend) == 0) {
                    data.emplace(current_block_key, current_block_content);
                    state = state_t::sNormal;
                } else {
                    current_block_content += line + '\n';
                }

                continue;
            }
        }

        return true;
    };
    bool read_file(const std::string& filename) {
        std::ifstream fstrm{filename};
        if (fstrm) {
            return read_stream(fstrm);
        }

        return false;
    };

    friend std::istream& operator >>(std::istream& strm, ConfigFile& cfg);
    std::istream& operator >>(std::istream& strm) {
        if (read_stream(strm)) {
            return strm;
        }

        throw std::runtime_error{std::string("read from stream failed:") + error()};
    }

    std::string operator[](const std::string& sv) const {
        std::cerr << "Looking up " << sv << "\n";
        return data.at(sv);
    };
};

std::istream& operator >>(std::istream& strm, ConfigFile & cfg) {
    if (!cfg.read_stream(strm)) {
        // do something - not sure what.:w
        //
    }
    return strm;
}

using command_ptr = int(*)(ConfigFile, ConfigFile);

int compile_fail_test(ConfigFile test_config, ConfigFile sys_config) {
    std::cout << "compile_fail_test -- starting\n";

    //
    // find a place to stash temps
    //
    auto temp_template_path = fs::temp_directory_path() / "test_runnerXXXXXX";

    //
    // Since mkdtemp scribbles into its parameter, we need to allocate a buffer
    // to store a non-const copy of the path::c_str(). Wrap it in a unique_ptr
    // to make sure we clean up on exit. However, we'll make a const copy of
    // the underlying pointer so we don't have to keep using .get()
    //
    auto template_buffer_size = strlen(temp_template_path.c_str()) +1;
    std::unique_ptr<char[]>temp_dir_ptr = std::make_unique<char[]>(template_buffer_size);
    memcpy(temp_dir_ptr.get(), temp_template_path.c_str(), template_buffer_size);

    const char * const temp_dir = temp_dir_ptr.get();

    if (mkdtemp(temp_dir_ptr.get()) != temp_dir) {
        std::cerr << "making temp_dir didn't work '" << temp_dir << "'\n";
        return 1;
    }
    std::cout << "working in directory : " << temp_dir << "\n";


    //
    // build the file to compile
    //
    fs::path input_file_path{temp_dir};
    input_file_path /= "input.cpp";
    std::ofstream source_code{(input_file_path).string()};

    if (not source_code) {
        std::cerr << "Wonder why we could not open the source file?\n";
        return 1;
    }

    source_code << test_config["code"];

    source_code.close();

    //
    // build the command line
    //
    fs::path error_file_path{temp_dir};
    error_file_path /= "error_out.txt";

    std::string command_line{sys_config["compiler"] + " "};

    command_line += sys_config["flags"] + " -I" + sys_config["include"] + " ";
    command_line += "-c " + input_file_path.string() + " > " + error_file_path.string() +
        " 2>&1";

    std::cerr << "command_line = " << command_line << "\n";

    auto retval = system(command_line.c_str());

    //
    // Check the return value
    //

    if (retval == 0) {
        std::cerr << "The compile failed to fail!\n";
        return 1;
    }

    //
    // Make sure the correct error message showed up.
    //
    std::ifstream error_msg_file{error_file_path.string()};

    std::stringstream error_msg_text;

    error_msg_text << error_msg_file.rdbuf();

    std::regex pattern{"input.cpp:" + test_config["line"] + ":.*" + test_config["regex"]};

    if (not std::regex_search(error_msg_text.str(), pattern)) {
        std::cerr << "Expected error message not found\n";
        return 1;
    }

    //
    // Now clean up our directory.
    //
    command_line = "rm -rf " + std::string(temp_dir);

    system(command_line.c_str());

    return 0;

}
std::map<std::string, command_ptr>CommandMap = {
    { "CXX_COMPILER", compile_fail_test },
};

int run_from_command_map(std::string cmd, ConfigFile test_config, ConfigFile sys_config) {

    std::cout << "run_from_command cmd = " << cmd << "\n";
    auto results = CommandMap.find(cmd);
    if ( results == CommandMap.end() ) {
        std::cerr << "Could not find " << cmd << " in the command map\n";
        return 1;
    }

    auto func = results->second;

    return (*func)(test_config, sys_config);
}

//
// Note that this exit()s from the progam if it fails
//
ConfigFile read_config_from_file(std::string fname) {
    std::ifstream i(fname);

    if (! i) {
        std::cerr << "Failed to open the config file '"<< fname << "'\n";
        exit(1);
    }

    ConfigFile cfg;

    try {
        i >> cfg;
    } catch (std::runtime_error& e) {
        std::cerr << "invalid config file:\n" << e.what() << "\n";
        exit(1);
    }

    return cfg;
}

int main(int argc, char *argv[], char* envp[]) {


    if (argc != 2) {
        std::cerr << "Expecting one argument (the config file)\n";

        return 1;
    }

    auto test_config = read_config_from_file(argv[1]);
    auto sys_config  = read_config_from_file("_runner_build_config.cfgfile");

    auto command = std::string(test_config["command"]);
    if (command[0] == ':') {
        command = command.substr(1);
    } else if (command[0] == '@') {
        command = "RUN_FROM_ENV";
    }

    return run_from_command_map(command, test_config, sys_config);
}
