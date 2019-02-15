#if !defined(YALR_AST_HPP)
#define YALR_AST_HPP

#include <boost/spirit/home/x3.hpp>
#include <boost/spirit/home/x3/support/ast/position_tagged.hpp>
#include <boost/fusion/include/adapt_struct.hpp>

#include <iostream>
#include <string>
#include <optional>
#include <vector>
#include <variant>

namespace yalr::ast {

    namespace x3 = boost::spirit::x3;

    struct symbol : x3::position_tagged {
        std::string name;
    };

    struct alternative : x3::position_tagged {
        std::string phony;
        std::vector<std::string>pieces;
    };

    struct rule_def : symbol {
        bool isgoal;
        std::string type_str;
        std::vector<alternative>alts;
    };

    struct terminal : symbol {
        std::string pattern;
        std::string type_str;
        std::string action;
    };

    struct skip : terminal {
    };

    struct grammar : x3::position_tagged {
        std::optional<std::string>parser_class;
        std::vector<std::variant<skip, terminal, rule_def>> defs;
        bool success;
    };

    void pretty_print(grammar& g, std::ostream& strm);

} // namespace yalr::ast

BOOST_FUSION_ADAPT_STRUCT(yalr::ast::alternative, phony, pieces);
BOOST_FUSION_ADAPT_STRUCT(yalr::ast::rule_def, isgoal, type_str, name, alts);
BOOST_FUSION_ADAPT_STRUCT(yalr::ast::terminal, type_str, name, pattern, action);
BOOST_FUSION_ADAPT_STRUCT(yalr::ast::skip, name, pattern);
BOOST_FUSION_ADAPT_STRUCT(yalr::ast::grammar, parser_class, defs);



#endif
