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

/****************************************************************************
 * ConfigFile
 *
 ****************************************************************************/
//
// Config file parsing
//
class ConfigFile {
    enum class state_t { sNormal, sBlock };
    std::string last_error = "";

    std::map<std::string, std::string>data;

  public :
    std::string error() const { return last_error; }

    bool add(std::string key, std::string value) {
        const auto &[_, s] = data.emplace(key, value);

        return s;
    }

    bool contains(std::string k) const {
        return (data.count(k) > 0);
    }

    static ConfigFile merge(const ConfigFile& left, const ConfigFile& right) {
        ConfigFile retval;

        for (const auto &[k, v] : left.data) {
            std::cout << "merge inserting " << k << "\n";
            retval.data.insert_or_assign(k, v);
        }
        for (const auto &[k, v] : right.data) {
            std::cout << "merge inserting " << k << "\n";
            retval.data.insert_or_assign(k, v);
        }

        std::cout << "merge total keys = " << retval.data.size() << "\n";
        return retval;
    }

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

/****************************************************************************
 * temp_directory_t
 *
 * Note: This is designed to be used in a RAII style:
 *  temp_directory_t tmp_dir{"prefix"};
 *
 * Note: This is not copyable.
 *
 ****************************************************************************/
class temp_directory_t {
    std::unique_ptr<char[]>temp_dir_ptr_;
    bool auto_remove_ = false;
    bool exists_ = false;

  public:
    temp_directory_t(std::string prefix, bool auto_remove=false) :
        auto_remove_(auto_remove),
        exists_(false) {


        auto temp_template_path = fs::temp_directory_path() / 
            (prefix + "XXXXXX");

        //
        // Since mkdtemp scribbles into its parameter, we need to allocate a buffer
        // to store a non-const copy of the path::c_str(). Wrap it in a unique_ptr
        // to make sure we clean up on exit. However, we'll make a const copy of
        // the underlying pointer so we don't have to keep using .get()
        //
        auto template_buffer_size = strlen(temp_template_path.c_str()) +1;
        temp_dir_ptr_ = std::make_unique<char[]>(template_buffer_size);
        memcpy(temp_dir_ptr_.get(), temp_template_path.c_str(), template_buffer_size);

        const char * const temp_dir = temp_dir_ptr_.get();


        if (mkdtemp(temp_dir_ptr_.get()) != temp_dir) {
            throw std::runtime_error{std::string("making temp_dir didn't work")};
        }

        exists_ = true;

        std::cout << "working in directory : " << temp_dir << "\n";
    }

    const char * const get() const { return temp_dir_ptr_.get(); }

    fs::path as_path() const { return get(); }

    void remove_dir() {
        if (exists_) {
            // switch to unlink at some point
            std::string command_line = "rm -rf " + std::string(get());
            system(command_line.c_str());
            exists_ = false;
        }
    }

    temp_directory_t(temp_directory_t && o) {
        using std::swap;
        swap(temp_dir_ptr_, o.temp_dir_ptr_);
        swap(exists_, o.exists_);
        auto_remove_ = o.auto_remove_;
        o.auto_remove_ = false;
    }
    temp_directory_t &operator=(temp_directory_t && o) {
        using std::swap;
        swap(temp_dir_ptr_, o.temp_dir_ptr_);
        swap(exists_, o.exists_);
        swap(auto_remove_, o.auto_remove_);

        return *this;
    }

    ~temp_directory_t() {
        if (auto_remove_ && exists_) {
            remove_dir();
        }
    }
};

/****************************************************************************
 * replace_config
 *
 * Build a string by replacing things of the form ${..} with values from the
 * passed ConfigFile.
 *
 ****************************************************************************/
std::string replace_config(std::string_view input, const ConfigFile &cfg) {
    
    std::string output;

    while (not input.empty()) {
        std::cout << "rc - looking for var to replace\n";
        auto pos = input.find("${");
        if (pos == std::string_view::npos) {
            output += std::string(input);
            break;
        } else if (pos > 0) {
            output += std::string(input.substr(0, pos));
            input.remove_prefix(pos);
        }

        std::cout << "rc - found at pos " << pos << "\n";
        // get rid of the opener.
        auto var_start = input.substr(2);

        pos = var_start.find('}');

        if (pos == std::string_view::npos) {
            // No closer, so just put out the rest of the string
            // and go home.
            std::cout << "rc - no closer found\n";
            output += std::string(input);
            input = "";
            break;
        }

        auto var = var_start.substr(0, pos);
        std::string var_key = std::string(var);

        //
        // This throws if the key doesn't exist
        //
        output += cfg[var_key];

        input.remove_prefix(pos+2+1);

    }

    return output;
}

/****************************************************************************
 * compile_fail_test
 *
 ****************************************************************************/
int compile_fail_test(ConfigFile test_config, ConfigFile sys_config) {
    std::cout << "compile_fail_test -- starting\n";

    temp_directory_t temp_dir_obj{"test_runner"};
    auto temp_dir = temp_dir_obj.get();

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

    command_line += sys_config["flags"] + " " + sys_config["include"] + " ";
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
    temp_dir_obj.remove_dir();

    return 0;

}

/****************************************************************************
 * run_command_line
 *
 * Runs the specified command and checks the results.
 * test_config keys that must exist:
 *      command_line = The detailed command to run
 *          This will look something like:
 *              ${cmd} ${flags} etc -o ${output_file} ${input_file}
 *          All referenced variables will need to be in either the
 *          sys config or test config - EXCEPT - ${output_file} and
 *          ${input_file}. These will be provided by the system.
 *          ${output_file} will be the file who's content will be checked
 *          for the given regex. ${input_file} is the path where the value
 *          of the `input` config is written.
 *      input = The contents for the input file. Normally this would be a block
 *          variable.
 *      regex = What to look for in the output_file to register success.
 ****************************************************************************/
int run_command_line(ConfigFile test_config, ConfigFile sys_config) {
    std::cout << "run_command_line -- starting\n";

    temp_directory_t temp_dir_obj{"test_runner"};

    auto full_config = ConfigFile::merge(test_config, sys_config);
    //
    // build the file to compile
    //
    fs::path input_file_path = temp_dir_obj.as_path() / "test_runner_input";

    std::ofstream input_file_stream{(input_file_path).string()};

    full_config.add("input_file", input_file_path.string());

    if (not input_file_stream) {
        std::cerr << "Wonder why we could not open the input_file_stream file?\n";
        return 1;
    }

    input_file_stream << test_config["input"];

    input_file_stream.close();

    //
    // Path for the output
    //
    fs::path output_file_path = temp_dir_obj.as_path() / "test_runner_output";

    full_config.add("output_file", output_file_path.string());

    //
    // Build command line
    //
    auto command_line = replace_config(test_config["command_line"], full_config);

    std::cout << "   command line = " << command_line << "\n";

    auto retval = system(command_line.c_str());

    //
    // Check the return value
    //

    if (retval == 1) {
        std::cerr << "Command Failed!\n";
        return 1;
    }

    //
    // Make sure the content is correct.
    //
    std::ifstream output_file{output_file_path.string()};

    std::stringstream output_file_content;

    output_file_content << output_file.rdbuf();

    std::regex pattern{full_config["regex"]};

    if (not std::regex_search(output_file_content.str(), pattern)) {
        std::cerr << "Expected content message not found\n";
        return 1;
    }

    //
    // Now clean up our directory.
    //
    temp_dir_obj.remove_dir();

    return 0;
}

/****************************************************************************/
using command_ptr = int(*)(ConfigFile, ConfigFile);

std::map<std::string, command_ptr>CommandMap = {
    { "CXX_COMPILER", compile_fail_test },
    { "COMMAND_LINE", run_command_line },
};
/****************************************************************************/

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
    std::ifstream istrm(fname);

    if (! istrm) {
        std::cerr << "Failed to open the config file '"<< fname << "'\n";
        exit(1);
    }

    ConfigFile cfg;

    try {
        istrm >> cfg;
    } catch (std::runtime_error& e) {
        std::cerr << "invalid config file:\n" << e.what() << "\n";
        exit(1);
    }

    return cfg;
}

ConfigFile read_config_from_args(int argc, char *argv[]) {
    ConfigFile cfg;

    // we assume the people calling us know what
    // they are doing. so don't try too hard;
    for(int i = 0; i< argc; ++i) {
        std::string_view sv = argv[i];
        auto pos = sv.find_first_of('=');
        if (pos == std::string_view::npos) {
            std::cerr << "Could not find an '=' in the argument:\n"
                << "   " << sv << "\n";
            exit(1);
        }
        auto key = sv.substr(0, pos);
        auto value = sv.substr(pos+1);

        if (value[0] == '\'' and value[value.size()-1] == '\'') {
            value = value.substr(1, value.size()-2);
        }

        cfg.add(std::string(key), std::string(value));
    }

    return cfg;
}

int main(int argc, char *argv[], char* envp[]) {


    if (argc < 2) {
        std::cerr << "Expecting at least one argument (the config file)\n";

        return 1;
    }

    auto test_config = read_config_from_file(argv[1]);
    auto sys_config  = read_config_from_args(argc-2, &argv[2]);

    auto command = std::string(test_config["command"]);
    if (command[0] == ':') {
        command = command.substr(1);
    } else if (command[0] == '@') {
        command = "RUN_FROM_ENV";
    }

    return run_from_command_map(command, test_config, sys_config);
}
