#include "ta0.hpp"

int main() {
    std::string input = R"T(
skip WS r:\s+  ;
term foo 'xyz' ; // does this work?
term <int> NUM r:[1-9][0-9]* <%{ return atoi(lexeme.c_str()); }%>
term COMMA ',' ;

// hopefully
rule <std::string> A {
    => NUM ;
    => A ',' NUM ;
}
)T";

    YalrParser::Lexer l(input.cbegin(), input.cend());
    auto parser = YalrParser::YalrParser(l);
    parser.debug = true;

    if (parser.doparse()) {
        std::cout << "It Worked!\n";
        return 0;
    } else {
        std::cout << "Too bad!\n";
        return 1;
    }
}
