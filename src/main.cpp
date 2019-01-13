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

// I'm not in love with the fact that
// you have to wrap the enitre script body into a try {}.
// And that you have no choice but to do everything up here.
// The reason is that cxxopts::ParseResult doesn't have a default
// constructor, so that you cannot do:
// cxxopts::ParseResult foo;
// try {
//    foo = options.parse(...);
// } catch ...


struct CLIOptions {
    std::string output_file;
    std::string translate;
    std::string input_file;
    bool help = false;
};

CLIOptions parse_commandline(int argc, char**argv) {
  
    CLIOptions clopts;

    try {
        cxxopts::Options options("yalr", "Parser Generator");

        options.positional_help("<input-file>");

        options.add_options()
            ("h,help", "Print help message", cxxopts::value(clopts.help))
            ("o,output-file", "File in which to put the main output", cxxopts::value(clopts.output_file))
            ("t,translate", "Output the grammar in another format", cxxopts::value(clopts.translate))
            ;
        options.add_options("positionals")
            ("input-file", "grammar file to process",  cxxopts::value(clopts.input_file))
            ;


        options.parse_positional({"input-file"});

        auto results = options.parse(argc, argv);

        // Grrr. have to handle help here because we need the options object.
        if (clopts.help) {
            std::cout << options.help() << "\n";
            exit(1);
        }

    } catch (const cxxopts::OptionException& e) {
        std::cerr << "Well, that didn't work: " << e.what() << "\n";
        exit(1);
    }

    return clopts;

}

//****************************
// Main
//****************************
int main(int argc, char* argv[]) {

    auto clopts = parse_commandline(argc, argv);

    if (clopts.input_file.empty()) {
        std::cerr << "Need something to parse\n";
        return 1;
    }

    std::ifstream in(clopts.input_file, std::ios_base::in);
    in.unsetf(std::ios_base::skipws);

    const std::string input_string(std::istreambuf_iterator<char>{in}, {});


    yalr::parser::iterator_type iter = input_string.begin();
    yalr::parser::iterator_type const eof = input_string.end();


    auto tree = yalr::do_parse(iter, eof);

    if (!tree.success) {
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
    if (not clopts.output_file.empty()) {
        outfilename = clopts.output_file;
    } else {
        outfilename = ana_tree->parser_class + ".hpp";
    }

    if (not clopts.translate.empty()) {
        const auto format = clopts.translate;

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
