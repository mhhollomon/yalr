#include "parser.hpp"
#include "analyzer.hpp"
#include "tablegen.hpp"
#include "codegen.hpp"

#include <gsl/span>

#include <iostream>
#include <fstream>
#include <ios>

//****************************
// Main
//****************************
int main(int argc, char* argv[])
{
    if (argc < 2) {
        std::cerr << "Need something to parse\n";
        return 1;
    }

    gsl::span<char *> args{argv, argc};

    std::ifstream in(args[1], std::ios_base::in);
    in.unsetf(std::ios_base::skipws);


    yalr::parser::iterator_type iter(in);
    yalr::parser::iterator_type const eof;


    auto [r, tree] = yalr::do_parse(iter, eof);

    if (!r) {
        std::cout << "Parse failed\n";
        std::cout << "Stopped at : ";
        for (int i =0; i < 20; ++i) {
            if (iter == eof) {
                break;
            }

            std::cout << *iter;
            ++iter;
        }
        std::cout << std::endl;

        return 1;
    } 
    
    if (iter != eof) {
        std::cout << "Failed: didn't parse everything\n";
        std::cout << "Stopped at : ";
        for (int i =0; i < 20; ++i) {
            if (iter == eof) {
                break;
            }

            std::cout << *iter;
            ++iter;
        }
        std::cout << std::endl;

        return 1;
    } 

    std::cout << "Good input\n";

    std::cout << "------ AST ------\n";
    yalr::ast::pretty_print(tree, std::cout);
    std::cout << "------ AST ------\n";

    auto ana_tree = yalr::analyzer::analyze(tree);

    std::cout << "------ ANALYZE ------\n";
    yalr::analyzer::pretty_print(*ana_tree, std::cout);
    std::cout << "------ ANALYZE ------\n";

    auto lrtbl = yalr::tablegen::generate_table(*ana_tree);

    std::cout << "------ TABLEGEN ------\n";
    yalr::tablegen::pretty_print(*lrtbl, std::cout);
    std::cout << "------ TABLEGEN ------\n";

    std::string outfilename;
    if (argc > 3) {
        outfilename = args[2];
    } else {
        outfilename = ana_tree->parser_class + ".hpp";
    }

    std::cout << "--- Generating code into " << outfilename << "\n";
    std::ofstream code_out(outfilename, std::ios_base::out);
    yalr::codegen::generate_code(*lrtbl, code_out);

    return 0;
}
