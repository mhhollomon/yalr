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

    struct alternative : x3::position_tagged {
        std::string phony;
        std::vector<std::string>pieces;
    };

    struct rule_def : x3::position_tagged {
        bool isgoal;
        std::string name;
        std::vector<alternative>alts;
    };

    struct terminal : x3::position_tagged {
        std::string name;
        std::string pattern;
    };

    struct grammar : x3::position_tagged {
        std::optional<std::string>parser_class;
        std::vector<std::variant<terminal, rule_def>> defs;
        bool success;
    };

    void pretty_print(grammar& g, std::ostream& strm);

} // namespace yalr::ast

BOOST_FUSION_ADAPT_STRUCT(yalr::ast::alternative, phony, pieces);
BOOST_FUSION_ADAPT_STRUCT(yalr::ast::rule_def, isgoal, name, alts);
BOOST_FUSION_ADAPT_STRUCT(yalr::ast::terminal, name, pattern);
BOOST_FUSION_ADAPT_STRUCT(yalr::ast::grammar, parser_class, defs);



#endif
