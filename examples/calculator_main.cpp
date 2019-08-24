#include "calculator.yalr.hpp"

int main() {
    std::string input = "3.0 + 5.6 / 2";

    YalrParser::Lexer l(input.cbegin(), input.cend());
    auto parser = YalrParser::Parser(l);

    //lexer.debug  = true;
    //parser.debug = true;

    if (parser.doparse()) {
        //std::cout << "Input matches grammar!\n";
        return 0;
    } else {
        std::cout << "Input does NOT match grammar\n";
        return 1;
    }
}
