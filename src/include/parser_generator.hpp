#pragma once

#include "analyzer_tree.hpp"
#include "utils.hpp"
#include "options.hpp"
#include "symbols.hpp"
#include "errorinfo.hpp"
#include "code_renderer.hpp"

#include <string>

namespace yalr::algo {

// all parse tables will need these for correct code
// generation. So give them a base class.
struct base_parse_table {
    std::string version_string;
    option_table options;
    symbol_table symbols;
    std::multimap<std::string, std::string_view> verbatim_map;

};

struct gen_results {
    error_list errors;
    bool success;
};

struct parser_generator {
    virtual
        std::string algo_name() const = 0;

    virtual
        std::unique_ptr<gen_results> generate_table(const analyzer_tree& t) = 0;
    
    // this isn't quite right. There could be errors in code generation.
    // but it will work as a first pass.
    virtual
        std::unique_ptr<gen_results> generate_code(yalr::code_renderer &cr) = 0;

    virtual
        void output_parse_table(std::ostream & strm) =0;

    virtual
        ~parser_generator() = default;

};

} // namespace yalr::algo
