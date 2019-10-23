#include "CLIOptions.hpp"
#include "parser.hpp"
#include "analyzer.hpp"
#include "tablegen.hpp"
#include "codegen.hpp"
#include "translate.hpp"

#include "cxxopts.hpp"

#include <iostream>
#include <fstream>
#include <ios>

// I'm not in love with the fact that
// you have to wrap the entire function body into a try {}.
// And that you have no choice but to do everything up here.
// The reason is that cxxopts::ParseResult doesn't have a default
// constructor, so that you cannot do:
// cxxopts::ParseResult foo;
// try {
//    foo = options.parse(...);
// } catch ...



CLIOptions parse_commandline(int argc, char**argv) {
  
    CLIOptions clopts;

    try {
        cxxopts::Options options("yalr", "Parser Generator");

        options.positional_help("<input-file>");

        options.add_options()
            ("h,help", "Print help message", cxxopts::value(clopts.help))
            ("o,output-file", "File in which to put the main output", cxxopts::value(clopts.output_file))
            ("S,state-table", "File in which to put the state table", 
                cxxopts::value(clopts.state_file)->implicit_value("-NONE :^-") )
            ("t,translate", "Output the grammar in another format", cxxopts::value(clopts.translate))
            ("d,debug", "Print debug information", cxxopts::value(clopts.debug))
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

    auto source = std::make_shared<yalr::text_source>(clopts.input_file, 
            std::string(std::istreambuf_iterator<char>{in}, {}));


    auto p = yalr::yalr_parser(source);
    auto tree =  p.parse();

    if (!tree.success) {
        std::cerr << "Parse failed\n";
        for (auto const& e : tree.errors) {
            e.output(std::cerr);
        }
        return 1;
    } 
    
    if (clopts.debug) {
        std::cout << "------ PARSE TREE ----------\n";
        tree.pretty_print(std::cout);
        std::cout << "------ PARSE TREE end ------\n";
    }

    auto anatree = yalr::analyzer::analyze(tree);
    if (not anatree->success) {
        for (auto const& e : anatree->errors) {
            e.output(std::cerr);
        }
        exit(1);
    }

    if (clopts.debug) {
        std::cout << "------ ANALYZE ------\n";
        yalr::analyzer::pretty_print(*anatree, std::cout);
        std::cout << "------ ANALYZE ------\n";
    }


    std::string outfilename;
    if (not clopts.output_file.empty()) {
        outfilename = clopts.output_file;
    } else {
        outfilename = clopts.input_file + ".hpp";
    }

    if (not clopts.translate.empty()) {
        const auto format = clopts.translate;

        if (format == "grammophone") {
            yalr::translate::grammophone().output(*anatree, clopts);
        } else {
            std::cerr << "Unknown format '" << format << "'\n";
            exit(1);
        }

        exit(0);
    }

    auto lrtbl = yalr::generate_table(*anatree);

    std::string state_file_name;
    if (not clopts.state_file.empty()) {
        if (clopts.state_file == "-NONE :^-") {
            state_file_name = outfilename + ".state.txt";
        } else {
            state_file_name = clopts.state_file;
        }
        std::cout << "--- Generating state table into " << state_file_name << "\n";
        std::ofstream state_out(state_file_name, std::ios_base::out);
        yalr::pretty_print(*lrtbl, state_out);
    }

    //
    // exit after having a chance to dump the state table.
    // That will have the info needed to allow the user to fix the problems.
    //
    if (not lrtbl->success) exit(1);


    std::cout << "--- Generating code into " << outfilename << "\n";
    std::ofstream code_out(outfilename, std::ios_base::out);
    yalr::generate_code(*lrtbl, code_out);

    return 0;
}
