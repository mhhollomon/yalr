#if not defined(YALR_ANALYZER_TREE_HPP)
#define YALR_ANALYZER_TREE_HPP

#include "errorinfo.hpp"
#include "options.hpp"
#include "symbols.hpp"
#include "production.hpp"

namespace yalr {

    struct analyzer_tree {
        bool success;
        std::list<error_info>    errors;
        option_list              options;
        symbol_table             symbols;
        std::vector<production>  productions;
        production_identifier_t  target_prod;
        std::list<std::string>   atoms;
    };

} // namespace yalr
#endif
