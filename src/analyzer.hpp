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

    struct symbol {
        enum symbol_type { unknownSymbol, ruleSymbol, termSymbol };
        symbol_type stype;
        int symbol_id;
        symbol(symbol_type t, int id) : stype(t), symbol_id(id) {};
        symbol() = default;

        std::string type_name() const {
            switch(stype) {
                case unknownSymbol:
                    return "Unknown";
                case ruleSymbol:
                    return "Rule";
                case termSymbol:
                    return "Term";
                default:
                    assert(false);
            }

            assert(false);
        }
    };

    struct production {
        static int next_value;

        int prod_id = ++next_value;
        int rule_id;
        std::vector<symbol> syms;
        production(int id, std::vector<symbol>&& s) :
            rule_id(id), syms(std::move(s))
        {}
    };

    struct grammar {
        std::string parser_class;
        std::string goal;
        std::map<std::string, terminal>terms;
        std::map<std::string, int>rules_by_name;
        std::map<int, std::string>rules_by_id;
        std::vector<production> productions;
    };

    std::unique_ptr<grammar> analyze(const parser::ast_tree_type &tree);

    void pretty_print(const grammar& g, std::ostream& strm);
}}

#endif
