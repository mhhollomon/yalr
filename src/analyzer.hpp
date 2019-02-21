#if !defined(YALR_ANALYZER_HPP_)
#define YALR_ANALYZER_HPP_

#include "ast.hpp"
#include "SymbolTable.hpp"

#include <string>
#include <memory>
#include <map>

namespace yalr { namespace analyzer {

    struct production {
        static int next_value;

        int prod_id = ++next_value;
        SymbolTable::symbol rule;
        std::vector<SymbolTable::symbol> syms;

        production(SymbolTable::symbol r, std::vector<SymbolTable::symbol>&& s) :
            rule(r), syms(std::move(s))
        {}
    };

    struct grammar {
        std::string parser_class;
        std::string lexer_class;
        std::string code_namespace;
        std::string goal;
        std::vector<production> productions;
        int target_prod;
        SymbolTable syms;
    };

    std::unique_ptr<grammar> analyze(ast::grammar &tree);

    struct pp {
        std::ostream& strm;
        pp(std::ostream& s) : strm(s) {}

        void operator()(const grammar &g);
        void operator()(const production& p);
    };

    void pretty_print(const grammar& g, std::ostream& strm);
}}

#endif
