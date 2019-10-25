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
        std::list<error_info>    errors;
        option_list              options;
        symbol_table             symbols;
        std::multimap<std::string, std::string_view> verbatim_map;
        std::vector<production>  productions;
        production_identifier_t  target_prod;
        std::list<std::string>   atoms;

        void record_error(const std::string& msg, const text_fragment tf) {
            errors.emplace_back(msg, tf); 
        }

        template <class ...Args>
        void record_error(const text_fragment tf, Args&&... args) {
            errors.emplace_back(util::concat(args...), tf);
        }

        operator bool() const { return success; }
    };


} // namespace yalr
#endif
