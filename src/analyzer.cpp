#include "analyzer.hpp"

namespace yalr { namespace analyzer {

void analyze(parser::ast_tree_type &tree) {
    if (tree.parser_class.has_value()) {
        // use it;
    }
}

}}
