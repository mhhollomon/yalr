#include "cli_options.hpp"
#include "parser.hpp"
#include "analyzer.hpp"
#include "translate.hpp"
#include "algo/slr.hpp"
#include "codegen.hpp"
#include "parser_generator.hpp"

#include "cxxopts.hpp"

#include <iostream>
#include <fstream>
#include <ios>
#include <string>
#include "yalr_version.hpp"


void print_version(std::ostream &strm) {
    strm << yalr::yalr_version_string << "\n";
}

const std::map<std::string, std::shared_ptr<yalr::algo::parser_generator>>
    algorithms = {
        { "slr", std::make_shared<yalr::algo::slr_generator>() },
    };

cli_options parse_commandline(int argc, char**argv) {
  
    cli_options clopts;
    bool do_help = false;
    bool do_version = false;

    cxxopts::Options options("yalr", yalr::yalr_version_string);

    options.positional_help("<input-file>");

    options.add_options()
        ("h,help", "Print help message", cxxopts::value(do_help))
        ("o,output-file", "File in which to put the main output", cxxopts::value(clopts.output_file))
        ("S,state-table", "File in which to put the state table", cxxopts::value(clopts.state_file) )
        ("t,translate", "Output the grammar in another format", cxxopts::value(clopts.translate))
        ("d,debug", "Print debug information", cxxopts::value(clopts.debug))
        ("v,version", "Print version information", cxxopts::value(do_version))
        ("algorithm", "Select parser algorithm", cxxopts::value(clopts.algorithm)->default_value("slr"))
        ;
    options.add_options("positionals")
        ("input-file", "grammar file to process",  cxxopts::value(clopts.input_file))
        ;


    options.parse_positional({"input-file"});

    try {
        auto results = options.parse(argc, argv);


        if (do_help) {
            std::cout << options.help() << "\n";
            exit(1);
        }

        if (do_version) {
            print_version(std::cout);
            exit(0);
        }

        if (clopts.input_file.empty()) {
            std::cerr << "Need something to parse\n";
            std::cerr << options.help() << "\n";
            exit(1);
        }

        if (algorithms.count(clopts.algorithm) == 0) {
            std::cerr << "Unknown parsing algorithm '" << clopts.algorithm << "'\n";
            exit(1);
        }

        if (results.count("state-table") > 0) {
            if (clopts.state_file.empty()) {
                clopts.state_file = "-NONE :^-";
            }
        }

    } catch (const cxxopts::OptionException& e) {
        std::cerr << "Command line error: " << e.what() << "\n\n";
        std::cerr << options.help() << "\n";
        exit(1);
    }

    return clopts;

}


//****************************
// Main
//****************************
int main(int argc, char* argv[]) {

    auto clopts = parse_commandline(argc, argv);

    auto generator = algorithms.at(clopts.algorithm);

    std::ifstream in(clopts.input_file, std::ios_base::in);
    in.unsetf(std::ios_base::skipws);

    auto source = std::make_shared<yalr::text_source>(clopts.input_file, 
            std::string(std::istreambuf_iterator<char>{in}, {}));


    auto p = yalr::yalr_parser(source);
    auto tree =  p.parse();

    if (!tree.success) {
        std::cerr << "Parse failed\n";
        tree.errors.output(std::cerr);
        return 1;
    } 
    
    if (clopts.debug) {
        std::cout << "------ PARSE TREE ----------\n";
        tree.pretty_print(std::cout);
        std::cout << "------ PARSE TREE end ------\n";
    }

    auto anatree = yalr::analyzer::analyze(tree);
    if (not anatree->success) {
        anatree->errors.output(std::cerr);
        exit(1);
    }

    if (clopts.debug) {
        std::cout << "------ ANALYZE ------\n";
        yalr::analyzer::pretty_print(*anatree, std::cout);
        std::cout << "------ ANALYZE ------\n";
    }


    anatree->version_string = yalr::yalr_version_string;

    if (not clopts.translate.empty()) {
        const auto format = clopts.translate;

        if (format == "grammophone") {
            yalr::translate::grammophone().output(*anatree, clopts);
        } else if (format == "lexer") {
            yalr::translate::lexer_graph().output(*anatree, clopts);

        } else {
            std::cerr << "Unknown format '" << format << "'\n";
            return 1;
        }

        return 0;
    }

    auto gen_res = generator->generate_table(*anatree);

    std::string outfilename;
    if (not clopts.output_file.empty()) {
        outfilename = clopts.output_file;
    } else {
        outfilename = clopts.input_file + ".hpp";
    }

    std::string state_file_name;
    if (not clopts.state_file.empty()) {
        if (clopts.state_file == "-NONE :^-") {
            state_file_name = outfilename + ".state.txt";
        } else {
            state_file_name = clopts.state_file;
        }
        std::cout << "--- Generating state table into " << state_file_name << "\n";
        std::ofstream state_out(state_file_name, std::ios_base::out);
        generator->output_parse_table(state_out);

    }

    //
    // exit after having a chance to dump the state table.
    // That will have the info needed to allow the user to fix the problems.
    //
    if (not gen_res->success) {
        // output errors ?
        return 1;
    }


    std::cout << "--- Generating code into " << outfilename << "\n";
    std::ofstream code_out(outfilename, std::ios_base::out);
    auto results = yalr::generate_code(generator, *anatree, code_out);

    return (results->success? 0 : 1);
}
