#pragma once

#include "algo/slr_parse_table.hpp"
#include "parser_generator.hpp"

namespace yalr::algo {

struct slr_generator :parser_generator {
    virtual
        std::string algo_name() const override { return "slr"; } ;

    virtual
        std::unique_ptr<gen_results> generate_table(const analyzer_tree& t) override;
    
    // this isn't quite right. There could be errors in code generation.
    // but it will work as a first pass.
    virtual
        std::unique_ptr<gen_results> generate_code(yalr::code_renderer &cr) override;

    virtual
        void output_parse_table(std::ostream & strm) override;

    private:
        std::unique_ptr<slr_parse_table> lrtable;

};

} // namespace yalr::aglo
