#pragma once

#include "parser_generator.hpp"
#include "analyzer_tree.hpp"

#include <memory>

namespace yalr {

    std::unique_ptr<algo::gen_results> 
        generate_code(
            std::shared_ptr<yalr::algo::parser_generator> gen, const analyzer_tree &tree, std::ostream &strm);

} // namespace yalr

