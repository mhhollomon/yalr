#if !defined(YALR_AST_HPP)
#define YALR_AST_HPP

#include <iostream>
#include <string>
#include <optional>
#include <vector>
#include <variant>

namespace yalr::ast {


    struct location {
        long offset;
        long line;
        long column;
    };

    struct locatable {
        location loc;
    };

    struct symbol : locatable {
        std::string name;
        std::string type_str;

        virtual ~symbol() {}
    };

    struct alternative : locatable {
        std::vector<std::string>pieces;
    };

    struct rule_def : symbol {
        bool isgoal;
        std::vector<alternative>alts;
    };

    struct terminal : symbol {
        std::string pattern;
        std::string action;
    };

    struct skip : terminal {
    };

    using def_list_type = std::vector<std::variant<skip, terminal, rule_def>>;
    struct grammar {
        std::string parser_class;
        std::string lexer_class;
        std::string code_namespace;
        std::vector<std::variant<skip, terminal, rule_def>> defs;
        bool success;
    };

    void pretty_print(grammar& g, std::ostream& strm);

} // namespace yalr::ast

#endif
