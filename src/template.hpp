#if ! defined(YALR_TEMPLATE_HPP)
#define YALR_TEMPLATE_HPP

#include <inja/inja.hpp>


namespace yalr::codegen {
// Main file template
std::string main_template =
R"DELIM(/* build time date version goes here */
#include <iostream>
#include <vector>
#include <regex>

namespace {{namespace}} {

enum token_type {
## for entry in enums
    {{entry.name}} = {{entry.value}},
## endfor
};

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
## for pat in patterns
    { {{pat.pattern}}, {{pat.token}} },
##endfor
};


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
                    max_len = len;
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

class {{namespace}} {
    Lexer& lexer;
    Token la;
    std::deque<Token> tokstack;

    void printstack() {
        std::cerr << "[" << la.toktype << "]" ;
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
/************** states *****************/

## for state in states

    rettype state{{state.id}}() {
        std::cerr << "entering state {{state.id}}";

        rettype retval;

        switch (la.toktype) {
## for action in state.actions
            case {{action.token}} :
            {% if action.type == "shift" %}
                shift(); retval = state{{action.newstateid}}(); break;
            {% else if action.type == "reduce" %}
                std::cerr << "Reducing by : {{action.production}}\n";
                reduce({{action.count}}, {{action.symbol}});
              {% if action.count > 0 %}
                return { state_action::reduce, {{action.returnlevels}}, {{action.symbol}} };
              {% else %}
                retval = { state_action::reduce, 0 , {{action.symbol}} };
              {% endif %}
            {% else %}
                return { state_action::accept, 0 };
            {% endif %}
                break;
## endfor
            default: return { state_action::error };
        }

        while (retval.action == state_action::reduce) {
            if (retval.depth > 0) {
                retval.depth -= 1;
                std::cerr << "Leaving state {{state.id}} in middle of reduce\n";
                return retval;
            }
## if length(state.gotos) > 0
            switch(retval.symbol) {
## for goto in state.gotos
                case {{goto.symbol}} : retval = state{{goto.stateid}}(); break;
## endfor
            }
## endif
        }

        return retval;
    }
## endfor
/************** states *****************/

public:
    {{namespace}}(Lexer& l) : lexer(l){};

    bool doparse() {
        la = lexer.next_token();
        auto retval = state0();
        if (retval.action == accept) {
            return true;
        }

        return false;
    }
}; // class {{namespace}}

} // namespace {{namespace}}

)DELIM";



} // namespace yalr::codegen
#endif
