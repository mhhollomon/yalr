#if !defined(YALR_AST_HPP)
#define YALR_AST_HPP

#include <boost/fusion/include/adapt_struct.hpp>

#include <iostream>
#include <string>
#include <optional>
#include <vector>

namespace yalr { namespace ast {

    struct alternative {
        std::string phony;
        std::vector<std::string>pieces;
        //std::string pieces;
    };

    struct rule_def {
        std::string name;
        std::vector<alternative>alts;
    };

    struct terminal {
        int phony;
        std::string name;
    };

    struct grammar
    {
        std::optional<std::string>parser_class;
        std::vector<ast::terminal> terms;
        std::vector<rule_def>rules;
    };

    void pretty_print(grammar& g, std::ostream& strm);

}}

BOOST_FUSION_ADAPT_STRUCT(yalr::ast::alternative, phony, pieces);
BOOST_FUSION_ADAPT_STRUCT(yalr::ast::rule_def, name, alts);
BOOST_FUSION_ADAPT_STRUCT(yalr::ast::terminal, phony, name);
BOOST_FUSION_ADAPT_STRUCT(yalr::ast::grammar, parser_class, terms, rules);



#endif
