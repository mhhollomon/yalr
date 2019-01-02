#include "ast.hpp"
#include "grammar.hpp"


#include <iostream>
//#include <string>
#include <fstream>
#include <ios>

namespace x3 = boost::spirit::x3;

//****************************
// Main
//****************************
int main(int argc, char**argv)
{
    if (argc < 2) {
        std::cerr << "Need something to parse\n";
        return 1;
    }

    std::ifstream in(argv[1], std::ios_base::in);
    in.unsetf(std::ios_base::skipws);


    yalr::parser::iterator_type iter(in);
    yalr::parser::iterator_type const eof;


    auto [r, tree] = yalr::do_parse(iter, eof);

    if (!r) {
        std::cout << "Parse failed\n";
        std::cout << "Stopped at : ";
        for (int i =0; i < 20; ++i) {
            if (iter == eof)
                break;

            std::cout << *iter;
            ++iter;
        }
        std::cout << std::endl;

        return 1;
    } else if (iter != eof) {
        std::cout << "Failed: didn't parse everything\n";
        std::cout << "Stopped at : ";
        for (int i =0; i < 20; ++i) {
            if (iter == eof)
                break;

            std::cout << *iter;
            ++iter;
        }
        std::cout << std::endl;

        return 1;
    } else {
        std::cout << "Good input\n";

        yalr::ast::pretty_print(tree, std::cout);
        return 0;
    }
}
