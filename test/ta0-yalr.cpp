#include "ta0.hpp"

int main() {
    std::string input = R"T(
skip WS r:\s+  ;
term foo 'xyz' ; // does this work?

// hopefully
rule A {
    => foo ;
    => A foo ;
}
)T";

    YalrParser::Lexer l(input.cbegin(), input.cend());
    auto parser = YalrParser::YalrParser(l);

    if (parser.doparse()) {
        std::cout << "It Worked!\n";
        return 0;
    } else {
        std::cout << "Too bad!\n";
        return 1;
    }
}
