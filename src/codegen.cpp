#include "codegen.hpp"

#include "analyzer.hpp"

namespace yalr::codegen {

    using tablegen::action_type;

void generate_state_function( const tablegen::lrstate& state, 
        const tablegen::lrtable& lt, std::ostream& outstrm) {

    analyzer::pp printer{outstrm};
    std::string pfx = "    ";

    outstrm << pfx << "rettype state" << state.id << "() {\n";
    outstrm << pfx << pfx << "std::cerr << \"entering state " << state.id <<
        "\\n\";\n";

    outstrm << R"T(
        rettype retval;

        switch (la.toktype) {
)T";

    for (const auto& [sym, action] : state.actions) {

        outstrm << pfx << pfx << pfx << "case ";
        if (sym.name() == "$") {
            outstrm << "eoi" ;
        } else {
            outstrm << "TOK_" << sym.name();
        }
        outstrm << " :\n";
        switch (action.atype) {
            case action_type::shift :
                outstrm << pfx << pfx << pfx << pfx << 
                    "shift(); retval = state" <<
                    action.new_state_id << "(); break;\n";
                break;
            case action_type::reduce : {
                    const auto& prod  = lt.productions[action.prod_id];
                    outstrm << pfx << pfx << pfx << pfx <<
                        "std::cerr << \"Reducing by : ";
                    printer(prod);

                    outstrm << "\\n\";\n";
                    outstrm << pfx << pfx << pfx << pfx << "reduce(" <<
                            prod.syms.size() << ", TOK_" << prod.rule.name() << ");\n";
                    if (prod.syms.size() > 0) {
                        outstrm << pfx << pfx << pfx << pfx <<
                            "return { state_action::reduce, " <<
                            (prod.syms.size()-1) << ", " << 
                            prod.rule.id() << " };\n";
                    } else {
                        outstrm << pfx << pfx << pfx << pfx <<
                            "retval = { state_action::reduce, 0, " <<
                            prod.rule.id() << " };\n";
                    }
                }
                break;

            case action_type::accept :
                outstrm << pfx << pfx << pfx << pfx << 
                    "std::cerr << \"Accepting\\n\";\n";
                outstrm << pfx << pfx << pfx << pfx << 
                    "return { state_action::accept, 0 };\n";
                break;

            default :
                //NOLINTNEXTLINE
                assert(false);
                break;
        }
    }

    outstrm << pfx << pfx << pfx << "default: return { state_action::error };\n";
    outstrm << pfx << pfx << "};\n";

    outstrm << R"T(
        while (retval.action == state_action::reduce) {
            if (retval.depth > 0) {
                retval.depth -= 1;
)T";
    outstrm << pfx << pfx << pfx << pfx << "std::cerr << \"Leaving state " << 
        state.id << " in middle of reduce\\n\";";

    outstrm << R"T(
                return retval;
            }
)T";

    if (not state.gotos.empty()) {
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

    outstrm << "#include <iostream>\n"
               "#include <vector>\n"
               "#include <regex>\n"
               "\n"
               ;

    // open the name space
    outstrm << "namespace " << lt.parser_class << " {\n";

    std::vector<SymbolTable::symbol>terms;
    // dump the tokens
    outstrm << "\nenum token_type {\n";

    for (const auto &[sname, sym] : lt.syms) {
        if (sym.isterm()) {
            // terms go in the enum and the term list
            if (sname == "$") {
                outstrm << "    eoi = " << sym.id() << ",\n";
            } else {
                outstrm << "    TOK_" << sname << " = " << 
                    sym.id() << ",\n";
                terms.push_back(sym);
            }
        } else if (sym.isrule()) {
            // rules only go in the enum
            outstrm << "    TOK_" << sname << " = " << 
                sym.id() << ",\n";
        } else if (sym.isskip()) {
            // Skips only go in the term list
            terms.push_back(sym);
        } else {
            //NOLINTNEXTLINE
            assert(false);
        }
    }


    outstrm << "    undef = -1,\n"
               "    skip  = -10\n};\n\n";

    // dump the lexer
    outstrm << R"T(
struct Token {
    token_type toktype;
    Token(token_type t = undef) : toktype(t) {}
};

enum state_action { undefined, reduce, accept, error };

struct rettype {
    state_action action;
    int depth;
    int symbol;
    rettype(state_action a = undefined, int d = 0, int s = 0) :
        action(a), depth(d), symbol(s) {}
};

std::vector<std::pair<std::string, token_type>> patterns = {
)T";
    std::sort(terms.begin(), terms.end());

    for (const auto& sym : terms) {
        const ast::terminal* info_ptr = sym.getTerminalInfo();

        if (! info_ptr) {
            info_ptr = sym.getSkipInfo();
        }

        std::string pattern = info_ptr->pattern.empty() ?
            "\"\"" : info_ptr->pattern;

        outstrm << "  {"  << pattern << ", ";
        if (sym.isterm()) {
            outstrm << "TOK_" << info_ptr->name;
       } else {
            outstrm << "skip" ;
       }
       outstrm << " }, \n";
    }

    outstrm << "};\n";



    outstrm << R"T(
class Lexer {
public:
    using iter_type = std::string::const_iterator;
    Lexer(iter_type first, const iter_type last) :
        current(first), last(last) {

        for (const auto& [patt, tok] : patterns) {
            try {
                regex_list.emplace_back(
                    std::regex{patt},
                    tok
                    );
            } catch (std::regex_error &e) {
                std::cerr << "problem compling pattern " <<
                    patt << "\n";
                throw e;
            }
        }
    }

    virtual Token next_token() {
        if (current == last) {
            std::cout << "Returning token eoi\n";
            return eoi;
        }

        token_type ret_type = undef;
        std::size_t max_len = 0;

        for (const auto &[r, tt] : regex_list) {
            std::match_results<iter_type> mr;
            //std::cerr << "Matching for token # " << tt;
            if (std::regex_search(current, last, mr, r, 
                    std::regex_constants::match_continuous)) {
                auto len = mr.length(0);
                //std::cerr << " length = " << len << "\n";
                if ( len > max_len) {
                    max_len = mr.length();
                    ret_type = tt;
                }
            } else {
                //std::cerr << " - no match\n";
            }
        }
        if (max_len == 0) {
            current = last;
            ret_type = eoi;
        } else {
            current += max_len;
            if (ret_type == skip) {
                //std::cerr << "recursing due to skip\n";
                return next_token();
            }
        }

        std::cout << "Returning token = " << ret_type << "\n";
        return Token{ret_type};
    }
private:
    std::string::const_iterator current;
    const std::string::const_iterator last;
    std::vector<std::pair<std::regex, token_type>> regex_list;
};
)T";

    // define the class
    outstrm << "\n\nclass " << lt.parser_class << " {\n";
    outstrm << R"T(
    Lexer& lexer;
    Token la;
    std::deque<Token> tokstack;

    void printstack() {
        for (const auto& x : tokstack) {
            std::cerr << " " << x.toktype;
        }
        std::cerr << "\n";
    }
    void shift() {
        std::cerr << "Shifting " << la.toktype << "\n";
        tokstack.push_back(la);
        printstack();
        la = lexer.next_token();
    }

    void reduce(int i, token_type t) {
        std::cerr << "Popping " << i << " items\n";
        for(; i>0; --i) {
            tokstack.pop_back();
        }
        std::cerr << "Shifting " << t << "\n";

        tokstack.push_back(t);
        
        printstack();
    }
)T";

    for (const auto& state : lt.states) {
        generate_state_function(state, lt, outstrm);
    }

    outstrm << "public:\n    " << lt.parser_class << "(Lexer& l) : lexer(l){};\n";
   
    outstrm << R"Tmplt(
    bool doparse() {
        la = lexer.next_token();
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

} //namespace yalr::codegen
