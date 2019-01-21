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
#include <algorithm>

namespace <%namespace%> {

enum token_type {
## for entry in enums
    <%entry.name%> = <%entry.value%>,
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

using iter_type = std::string::const_iterator;
struct matcher {
    virtual std::pair<bool, int>
    try_match(iter_type first, const iter_type last) = 0;

    virtual ~matcher() {}
};

struct string_matcher : matcher {
    std::string pattern;
    string_matcher(std::string p) : pattern{p} {};
    virtual std::pair<bool, int>
    try_match(iter_type first, const iter_type last) override {
        if (last - first < pattern.size()) {
            return std::make_pair(false, 0);
        } else if (std::equal(pattern.begin(), pattern.end(), first)) {
            return std::make_pair(true, pattern.size());
        } else {
            return std::make_pair(false, 0);
        }
    }
};

struct regex_matcher : matcher {
    std::regex pattern;
    regex_matcher(std::string p) try : pattern{p} {
         {}
    } catch (std::regex_error &e) {
        std::cerr << "Error when compiling pattern '" << p << "'\n";
        throw e;
    }
    virtual std::pair<bool, int>
    try_match(iter_type first, const iter_type last) override {
        std::match_results<iter_type> mr;
        if (std::regex_search(first, last, mr, pattern, 
                std::regex_constants::match_continuous)) {
            auto len = mr.length(0);
            return std::make_pair(true, len);
        } else {
            return std::make_pair(false, 0);
        }
    }
};

using match_ptr = std::shared_ptr<matcher>;

std::vector<std::pair<match_ptr, token_type>> patterns = {
## for pat in patterns
    { std::make_shared<<%pat.matcher%>>( <%pat.pattern%> ), <%pat.token%> },
##endfor
};


class Lexer {
public:
    Lexer(iter_type first, const iter_type last) :
        current(first), last(last) {
    }

    virtual Token next_token() {
        if (current == last) {
            std::cout << "Returning token eoi\n";
            return eoi;
        }

        token_type ret_type = undef;
        std::size_t max_len = 0;

        for (const auto &[m, tt] : patterns) {
            //std::cerr << "Matching for token # " << tt;
            auto [matched, len] = m->try_match(current, last);
            if (matched) {
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

    // Just needed to make it virtual
    virtual ~Lexer() = default;
private:
    std::string::const_iterator current;
    const std::string::const_iterator last;
};

class <%namespace%> {
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

    rettype state<%state.id%>() {
        std::cerr << "entering state <%state.id%>\n";

        rettype retval;

        switch (la.toktype) {
## for action in state.actions
            case <%action.token%> :
            {% if action.type == "shift" %}
                shift(); retval = state<%action.newstateid%>(); break;
            {% else if action.type == "reduce" %}
                std::cerr << "Reducing by : <%action.production%>\n";
                reduce(<%action.count%>, <%action.symbol%>);
              {% if action.count > 0 %}
                return { state_action::reduce, <%action.returnlevels%>, <%action.symbol%> };
              {% else %}
                retval = { state_action::reduce, 0 , <%action.symbol%> };
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
                std::cerr << "Leaving state <%state.id%> in middle of reduce\n";
                return retval;
            }
## if length(state.gotos) > 0
            switch(retval.symbol) {
## for goto in state.gotos
                case <%goto.symbol%> : retval = state<%goto.stateid%>(); break;
## endfor
            }
## endif
        }

        return retval;
    }
## endfor
/************** states *****************/

public:
    <%namespace%>(Lexer& l) : lexer(l){};

    bool doparse() {
        la = lexer.next_token();
        auto retval = state0();
        if (retval.action == accept) {
            return true;
        }

        return false;
    }
}; // class <%namespace%>

} // namespace <%namespace%>

)DELIM";



} // namespace yalr::codegen
#endif
