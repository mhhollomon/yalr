#include "calculator.yalr.hpp"
#include <string>

int main(int argc, char*argv[]) {
    std::string input = 
        R"(a := 5 * 3.0 + 5.6 / 2
print a
b := a+1
PRint b
)";


    YalrParser::Lexer lexer(input.cbegin(), input.cend());
    auto parser = YalrParser::Parser(lexer);

    if (argc > 1) {
       if (std::string("-p").compare(argv[1]) == 0) {
            parser.debug = true;
       } else if (std::string("-l").compare(argv[1]) == 0) {
            lexer.debug = true;
       } else if (std::string("-b").compare(argv[1]) == 0) {
            parser.debug = true;
            lexer.debug = true;
       }
    }

    if (parser.doparse()) {
        //std::cout << "Input matches grammar!\n";
        return 0;
    } else {
        std::cout << "Input does NOT match grammar\n";
        return 1;
    }
}
