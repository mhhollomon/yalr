#include "parser.hpp"
#include "analyzer.hpp"
#include "tablegen.hpp"
#include "codegen.hpp"
#include "translate.hpp"

#include <cxxopts/cxxopts.hpp>
#include <gsl/span>

#include <iostream>
#include <fstream>
#include <ios>

cxxopts::Options create_options() {
    cxxopts::Options options("yalr", "Parser Generator");

    options.positional_help("<input-file>");

    options.add_options()
        ("h,help", "Print help message")
        ("o,output-file", "File in which to put the main output", 
            cxxopts::value<std::string>())
        ("t,translate", "Output the grammar in another format",
            cxxopts::value<std::string>())
        ;
    options.add_options("positionals")
        ("input-file", "grammar file to process",  cxxopts::value<std::string>())
        ;


    options.parse_positional({"input-file"});


    return options;
}

//****************************
// Main
//****************************
int main(int argc, char* argv[])
{
    auto options = create_options();

    auto results = options.parse(argc, argv);

    if (results.count("help"))
    {
      std::cout << options.help() << std::endl;
      exit(0);
    }

    if (not results.count("input-file")) {
        std::cerr << "Need something to parse\n";
        return 1;
    }

    //gsl::span<char *> args{argv, argc};

    std::ifstream in(results["input-file"].as<std::string>(), std::ios_base::in);
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

    std::string outfilename;
    if (results.count("output-file")) {
        outfilename = results["output-file"].as<std::string>();
    } else {
        outfilename = ana_tree->parser_class + ".hpp";
    }

    if (results.count("translate")) {
        const auto format = results["translate"].as<std::string>();

        if (format == "grammophone") {
            std::ofstream code_out(outfilename, std::ios_base::out);
            yalr::translate::grammophone(*ana_tree).output(code_out);
        } else {
            std::cerr << "Unkown format '" << format << "'\n";
            exit(1);
        }

        exit(0);
    }

    auto lrtbl = yalr::tablegen::generate_table(*ana_tree);

    std::cout << "------ TABLEGEN ------\n";
    yalr::tablegen::pretty_print(*lrtbl, std::cout);
    std::cout << "------ TABLEGEN ------\n";

    std::cout << "--- Generating code into " << outfilename << "\n";
    std::ofstream code_out(outfilename, std::ios_base::out);
    yalr::codegen::generate_code(*lrtbl, code_out);

    return 0;
}
