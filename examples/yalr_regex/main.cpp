#include "regex.hpp"
#include <string>
#include <fstream>

int main(int argc, char*argv[]) {

    int current_arg = 1;
    bool parser_debug = false;
    bool lexer_debug = false;
    bool input_is_file = false;
    std::string regex = "";
    std::string file = "";

    while (argc > current_arg) {

       if (std::string("-p").compare(argv[current_arg]) == 0) {
            parser_debug = true;
            current_arg += 1;
       } else if (std::string("-l").compare(argv[current_arg]) == 0) {
            lexer_debug = true;
            current_arg += 1;
       } else if (std::string("-b").compare(argv[current_arg]) == 0) {
            parser_debug = true;
            lexer_debug = true;
            current_arg += 1;
       } else if (std::string("-f").compare(argv[current_arg]) == 0) {
            current_arg += 1;
            if (argc > current_arg) {
               file = std::string(argv[current_arg]);
               current_arg += 1;
               input_is_file = true;
            } else {
                std::cerr << "File name must be given for the -f option\n";
                exit(1);
            }
        } else {
            regex = argv[current_arg];
            break;
        }
    }

    if (regex.empty()) {
        std::cerr << "No regex given\n";
        exit(1);
    }

    YalrParser::Lexer lexer(regex.cbegin(), regex.cend());
    lexer.debug = lexer_debug;

    auto parser = YalrParser::Parser(lexer);
    parser.debug = parser_debug;

    if (!parser.doparse()) {
        std::cout << "Input does NOT match grammar\n";
        return 1;
    }

    rpn_ptr rpn_list = parser.get_semantic_value();

    dump_list(std::cout, rpn_list);

    /***************************************/
    exit(0);
    /***************************************/

    /*
    if (input_is_file) {
        if (input == "-") {
            std::ostringstream os;
            os << std::cin.rdbuf();
            input = os.str();
        } else {
            // slurp the entire file back into input
            //
            std::ifstream fstrm{input};
            if (fstrm) {
                std::ostringstream os;
                os << fstrm.rdbuf();
                input = os.str();
            } else {
                std::cerr << "Failed to open file '" << input << "'\n";
                exit(1);
            }
        }
    }
    */

}
