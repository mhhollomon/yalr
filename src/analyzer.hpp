#if !defined(YALR_ANALYZER_HPP_)
#define YALR_ANALYZER_HPP_

#include "parser.hpp"

#include <string>
#include <memory>
#include <map>

namespace yalr { namespace analyzer {

    struct terminal {
        static int next_value;

        std::string name;
        int value = ++next_value ;

        terminal(const std::string n) :name{n} {}
    };

    struct alternative {
        std::vector<std::string> pieces;

        alternative(const ast::alternative& ast) : pieces(ast.pieces) {}
    };

    struct rule_def {
        std::string name;
        std::vector<alternative>alts;

        rule_def(const ast::rule_def& ast) : name(ast.name) {
            for (const auto& a : ast.alts) {
                alts.push_back(a);
            }
        }
    };

    struct grammar {
        std::string parser_class;
        std::string goal;
        std::map<std::string, terminal>terms;
        std::map<std::string, rule_def>rules;
    };

    std::unique_ptr<grammar> analyze(const parser::ast_tree_type &tree);

    void pretty_print(const grammar &g, std::ostream& strm);
}}

#endif
