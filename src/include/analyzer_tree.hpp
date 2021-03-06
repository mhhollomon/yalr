#if not defined(YALR_ANALYZER_TREE_HPP)
#define YALR_ANALYZER_TREE_HPP

#include "utils.hpp"
#include "errorinfo.hpp"
#include "options.hpp"
#include "symbols.hpp"
#include "production.hpp"

#include <list>
#include <memory>
#include <map>
#include <unordered_set>

namespace yalr {

extern std::unordered_set<std::string_view> verbatim_locations;

    struct analyzer_tree {
        bool success;
        error_list               errors;
        option_table             options;
        symbol_table             symbols;
        std::multimap<std::string, std::string_view> verbatim_map;
        std::vector<production>  productions;
        production_identifier_t  target_prod;
        std::list<std::string>   atoms;

        template <class ...Args>
        error_info & record_error(const text_fragment tf, Args&&... args) {
            return errors.add(util::concat(args...), tf);
        }

        operator bool() const { return success; }
    };


} // namespace yalr
#endif
