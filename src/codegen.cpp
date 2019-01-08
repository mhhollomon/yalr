#include "codegen.hpp"

#include "analyzer.hpp"

namespace yalr { namespace codegen {

    using tablegen::action_type;

void generate_state_function( const tablegen::lrstate& state, 
        const tablegen::lrtable& lt, std::ostream& outstrm) {

    analyzer::pp printer{outstrm};

    std::string pfx = "    ";
    outstrm << pfx << "rettype state" << state.id << "() {\n";
    outstrm << pfx << pfx << "std::cout << \"entering state " << state.id <<
        "\\n\";\n";

    auto [_, eoi_sym] = lt.syms.find("$");

    const auto eoi_iter = state.actions.find(eoi_sym);

    // If there is an action on the eoi symbol,
    // then that takes preccedence. So, check that
    // first.
    if (eoi_iter != state.actions.end()) {
        const auto action = eoi_iter->second;
        if (action.atype == action_type::reduce) {
            const auto& prod  = lt.productions[action.prod_id];
            outstrm << pfx << pfx << "// Reduce\n";
            outstrm << pfx << pfx << "std::cout << \"reducing by : ";
            printer(prod);

            outstrm << "\\n\";\n" << pfx << pfx << "return { reduce, " <<
                    (prod.syms.size()-1) << "," << prod.rule.id() << " };\n" <<
                pfx << "}\n\n";
            return;
        } 

        if (action.atype == action_type::accept) {

            outstrm << R"T(
        Token tok = lexer.next_token();

        if (tok.toktype == eoi) {
            // Accept.
            return { accept, 0 };
        } else {
            // ERROR.
            return { error, 0 };
        }
    }

)T";
            return;
        }

        // there should only ever be reduce or accept on the
        // end-of-input symbol.
        assert(false);
    }

    outstrm << R"T(
        Token tok = lexer.next_token();
        rettype retval;

        switch (tok.toktype) {
)T";
    for (const auto& a_iter : state.actions) {
        const auto action = a_iter.second;
        const auto sym    = a_iter.first;

        if (sym != eoi_sym) {
            outstrm << pfx << pfx << pfx << 
                "case TOK_" << sym.name() << " : retval = state" <<
                action.new_state_id << "(); break;\n";
        }
    }

    outstrm << pfx << pfx << pfx << "default: return { error };\n";
    outstrm << pfx << pfx << "};\n";

    outstrm << R"T(
        while (retval.action == reduce) {
            if (retval.depth > 0) {
                retval.depth -= 1;
                return retval;
            }
)T";

    if (state.gotos.size() > 0) {
        outstrm << pfx << pfx << pfx << "switch(retval.symbol) {\n";
        for (const auto& g_iter : state.gotos) {
            outstrm << pfx << pfx << pfx << pfx <<
                "case " << g_iter.first.id() << 
                " : retval = state" << g_iter.second <<
                "(); break;\n";
        }
        outstrm <<  pfx << pfx << pfx << "}\n";
    }

    outstrm << R"T(
        }
        return retval;
    }
)T";
}

void generate_code(const tablegen::lrtable& lt, std::ostream& outstrm) {

    outstrm << "#include <iostream>\n";

    // open the name space
    outstrm << "namespace " << lt.parser_class << " {\n";

    // dump the tokens
    outstrm << "\nenum token_type {\n";
    for (const auto &s_iter : lt.syms) {
        if (s_iter.first == "$") {
            outstrm << "    eoi = " << s_iter.second.id() << ",\n";
        } else {
            outstrm << "    TOK_" << s_iter.first << " = " << 
                s_iter.second.id() << ",\n";
        }
    }
    outstrm << "    TOK_UNDEF = -1\n};\n\n";

    // dump the lexer
    outstrm << R"T(
struct Token {
    token_type toktype;
    Token(token_type t) : toktype(t) {}
};

enum state_action { undefined, reduce, accept, error };

struct rettype {
    state_action action;
    int depth;
    int symbol;
    rettype(state_action a = undefined, int d = 0, int s = 0) :
        action(a), depth(d), symbol(s) {}
};

class Lexer {
public:
    virtual Token next_token() {
        /* dummy */;
        return eoi;
    }
};
)T";

    // define the class
    outstrm << "class " << lt.parser_class << " {\n    Lexer& lexer;\n";

    for (const auto& state : lt.states) {
        generate_state_function(state, lt, outstrm);
    }

    outstrm << "public:\n    " << lt.parser_class << "(Lexer& l) : lexer(l){};\n";
   
    outstrm << R"Tmplt(
    bool doparse() {
        auto retval = state0();
        if (retval.action == accept) {
            return true;
        }

        return false;
    }
}; // class
} // namespace
)Tmplt";

}

}}
