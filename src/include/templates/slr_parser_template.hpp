#pragma once

#include <string>

namespace yalr::algo::slr {

using namespace std::string_literals;

const std::string slr_parser_template =
R"DELIM(

namespace <%namespace%> {  // for the lexer
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
        if (std::size_t(last - first) < pattern.size()) {
            return std::make_pair(false, 0);
        } else if (std::equal(pattern.begin(), pattern.end(), first)) {
            return std::make_pair(true, pattern.size());
        } else {
            return std::make_pair(false, 0);
        }
    }
};

struct fold_string_matcher : matcher {
    std::string pattern;
    fold_string_matcher(std::string p) : pattern{p} {};
    virtual std::pair<bool, int>
    try_match(iter_type first, const iter_type last) override {
        if (std::size_t(last - first) < pattern.size()) {
            return std::make_pair(false, 0);
        } else if (std::equal(pattern.begin(), pattern.end(), first, 
                [](char cA, char cB) {
                        return toupper(cA) == toupper(cB);
                   })) {
            return std::make_pair(true, pattern.size());
        } else {
            return std::make_pair(false, 0);
        }
    }
};

struct regex_matcher : matcher {
    std::regex pattern;
    regex_matcher(std::string p, const std::regex_constants::syntax_option_type& opt) try : pattern{p, opt} {
         {}
    } catch (std::regex_error &e) {
        std::cerr << "Error when compiling pattern '" << p << "'\n";
        throw e;
    }
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

std::vector<std::tuple<match_ptr, token_type, bool>> patterns = {
## for pat in patterns
    { std::make_shared<<%pat.matcher%>>( <%pat.pattern%> <%pat.flags%> ), <%pat.token%>, <%pat.is_global%> },
##endfor
};


class <%lexerclass%> {
/***** verbatim lexer.top ********/
## for v in verbatim.lexer_top
<% v %>
## endfor
/***** verbatim lexer.top ********/
public:
#if defined(YALR_DEBUG)
    bool debug = false;
#endif
    Lexer(iter_type first, const iter_type last) :
        current(first), last(last) {
    }

    virtual token next_token( std::optional<std::set<token_type>> allowed_tokens = std::nullopt) {
        if (current == last) {
            YALR_LDEBUG( "Returning token eoi\n");
            return eoi;
        }

        token_type ret_type = undef;
        std::size_t max_len = 0;
#if defined(YALR_DEBUG)
        if (debug) {
            std::cerr << "Next few characters: " ;
            auto ptr = current;
            for (int i =0 ; i < 10 ; ++i) {
                std::cerr << *ptr;
                ++ptr;
            }
            std::cerr << "\n";
        }
#endif

        for (const auto &[m, tt, glbl] : patterns) {
            if (not (glbl or !allowed_tokens or (allowed_tokens and allowed_tokens->count(tt) > 0))) {
                continue;
            }
            YALR_LDEBUG("Matching for token # " << tt << (glbl? " (global)" : ""));
            auto [matched, len] = m->try_match(current, last);
            if (matched) {
                YALR_LDEBUG(" length = " << len << "\n");
                if ( std::size_t(len) > max_len) {
                    max_len = len;
                    ret_type = tt;
                }
            } else {
                YALR_LDEBUG(" - no match\n");
            }
        }
        if (max_len == 0) {
            current = last;
            return token{undef};
        } else if (ret_type == skip) {
            YALR_LDEBUG("recursing due to skip\n");
            current += max_len;
            return next_token(allowed_tokens);
        }
        std::string lx{current, current+max_len};
        semantic_value ret_sval;
        switch (ret_type) {
## for sa in semantic_actions 
            case <%sa.token%> : {
                    auto block = [](std::string&& lexeme) -> <%sa.type%>
                    {  <%sa.block%> };
                    ret_sval = block(std::move(lx));
                }
                break;
## endfor
            default :
                /* do nothing */
                break;
        }

        if (debug) {
            std::string lx{current, current+max_len};
            YALR_LDEBUG( "Returning token = " << ret_type);
            if (ret_type >= 0) {
                YALR_LDEBUG(" (" << token_name[ret_type] << ")")
            }
            YALR_LDEBUG (" lex={" << lx << "}\n");
        }
        current += max_len;
        return token{ret_type, ret_sval};
    }

    // Just needed to make it virtual
    virtual ~Lexer() = default;
private:
    std::string::const_iterator current;
    const std::string::const_iterator last;

/***** verbatim lexer.bottom ********/
## for v in verbatim.lexer_bottom
<% v %>
## endfor
/***** verbatim lexer.bottom ********/
};


class <%parserclass%> {
    Lexer& lexer;
    token la;
    int current_state;

    struct parse_stack_entry {
        token tv;
        int         state;
    };

    std::deque<parse_stack_entry> tokstack;

    void printstack() {
        std::cerr << "(st=" << current_state << ")[la= " << la << "]" ;
        for (const auto& x : tokstack) {
            std::cerr << " (" << x.tv;
            std::cerr << ",s=" << x.state;
            std::cerr << ")";
        }
        std::cerr << "\n";
    }

/***** verbatim parser.top start ********/
## for v in verbatim.parser_top
<% v %>
## endfor
/***** verbatim parser.top end ********/

/************** reduce functions *****************/
    // A reduce function pointer
    // Returns a semantic value
    using reduce_func_ptr = parse_stack_entry (<%parserclass%>::*)();

## for func in reducefuncs
    parse_stack_entry reduce_by_prod<%func.prodid%>() {
        YALR_PDEBUG( "Reducing by : <%func.production%>\n");
        parse_stack_entry retval;
        semantic_value sv;

## for type in func.itemtypes
        {% if type.type != "void" -%} auto _v<%type.index%> = std::get<<%type.type%>>(tokstack.back().tv.v); {%- endif -%}
        {%- if type.alias != "" %}
        auto &<%type.alias%> = _v<%type.index%>; {%- endif -%}
        {%- if loop.is_last %}
        retval = tokstack.back(); {%- endif %}
        tokstack.pop_back();

## endfor
        {% if length(func.itemtypes) == 0 -%} retval.state = current_state; {%- endif %}
## if func.hassemaction == "Y"
        auto block = [&]() {
            <%func.block%>
        };
##   if func.rule_type != "void"
        sv = block();
##   else
        block();
##   endif
## endif

        retval.tv = { <%func.symbol%>, sv};
        return retval;
    }
## endfor
/************** end reduce functions *****************/

/************* parser table *************************/

    struct action_entry {
        // Our current state
        int state_id;
        //  What we just saw. This will only be a true terminal.
        token_type toktype;
        // What we should do
        state_action act_type;
        // shift vars
        int new_state_id;
        // reduce vars
        reduce_func_ptr reduce_func;
    };

    static const constexpr action_entry action_error_entry = {0, token_type::undef, state_action::error, 0, nullptr };


    std::vector<action_entry> state_table = {
## for state in states
## for action in state.actions
## if action.type == "shift"
        { <%state.id%> , <%action.token%> , state_action::<%action.type%> , <%action.newstateid%>, nullptr},
## else if action.type == "reduce"
        { <%state.id%>, <%action.token%>, state_action::<%action.type%>, 0, &<%parserclass%>::reduce_by_prod<%action.prodid%>},
## else if action.type == "accept"
        { <%state.id%> , <%action.token%> , state_action::<%action.type%>, 0, nullptr },
## endif
## endfor
## endfor
    };

    const action_entry &find_action(int state, token_type tt) {
        // binary search
        int bottom = 0;
        int top = state_table.size()-1;
            
        while(true) {
            if (top < bottom) {
                return action_error_entry;
            } else if (top == bottom) {
                if (state_table[top].state_id == state and state_table[top].toktype == tt) {
                    return state_table[top];
                } else {
                    return action_error_entry;
                }
            } else { // top > bottom 
                int index = (top+bottom)/2; // integer math
                auto &current = state_table[index];
                if ( state > current.state_id or (state == current.state_id and tt > current.toktype)) {
                    // index will equal bottom if top and bottom are 1 apart.
                    if (index == bottom) {
                        bottom += 1;
                    } else {
                        bottom = index;
                    }
                } else if (state == current.state_id and tt == current.toktype) {
                    // we hit pay dirt
                    return current;
                } else { // key is < suspect. look in lower half.
                    // integer math above ensures that, if we get here,
                    // index will be strictly less than top.
                    top = index;
                }
            }

        } /// while

        // should never get here. but just to make sure.
        // probably should be an assert.
        return action_error_entry;
    }

    struct goto_entry {
        int state_id;
        token_type toktype;
        int new_state_id;
    };

    std::vector<goto_entry> goto_table {
## for state in states
##   for goto in state.gotos
        { <%state.id%>, <%goto.symbol%>, <%goto.stateid%> },
##   endfor
## endfor
    };

    int find_goto(int state, token_type tt) {
        // binary search
        int bottom = 0;
        int top = goto_table.size()-1;
            
        while(true) {
            if (top < bottom) {
                return -1;
            } else if (top == bottom) {
                if (goto_table[top].state_id == state and goto_table[top].toktype == tt) {
                    return goto_table[top].new_state_id;
                } else {
                    return -1;
                }
            } else { // top > bottom 
                int index = (top+bottom)/2; // integer math
                auto &current = goto_table[index];
                if ( state > current.state_id or (state == current.state_id and tt > current.toktype)) {
                    // index will equal bottom if top and bottom are 1 apart.
                    if (index == bottom) {
                        bottom += 1;
                    } else {
                        bottom = index;
                    }
                } else if (state == current.state_id and tt == current.toktype) {
                    // we hit pay dirt
                    return current.new_state_id;
                } else { // key is < suspect. look in lower half.
                    // integer math above ensures that, if we get here,
                    // index will be strictly less than top.
                    top = index;
                }
            }

        } /// while

        // should never get here. but just to make sure.
        // probably should be an assert.
        return -1;
    }

    std::vector<std::set<token_type>> valid_terms = {
## for state in states
        {
            {% for act in state.actions -%}<%act.token%>, {%- endfor %}
        },
## endfor
    };

public:
#if defined(YALR_DEBUG)
    bool debug = false;
#endif
    <%parserclass%>(<%lexerclass%>& l) : lexer(l){};

    bool doparse() {
        current_state = 0;
        la = lexer.next_token(valid_terms[current_state]);
        bool done = false;
        while(not done) {
#if defined(YALR_DEBUG)
            if (debug) printstack();
#endif
            auto action = find_action(current_state, la.t);
            switch(action.act_type) {
                case state_action::accept :
                    YALR_PDEBUG("$$$$$$$ Accepting $$$$\n");
                    return true;
                case state_action::shift :
                    YALR_PDEBUG("shifting and going to state " << action.new_state_id << "\n");
                    tokstack.push_back({la, current_state});
                    current_state = action.new_state_id;
                    la = lexer.next_token(valid_terms[current_state]);
                    break;
                case state_action::reduce :
                    {
                    auto pse = std::invoke(action.reduce_func, *this);
                    auto new_state = find_goto(pse.state, pse.tv.t);
                    YALR_PDEBUG("Goto - old state = " <<pse.state <<" new state = " << new_state << "\n");
                    tokstack.push_back(pse);
                    current_state = new_state;
                    }
                    break;
                default:
                    YALR_PDEBUG("!!!!! ERROR !!!!");
                    return false;
            }
        }

        return false;
    }

/***** verbatim parser.bottom ********/
## for v in verbatim.parser_bottom
<% v %>
## endfor
/***** verbatim parser.bottom ********/

}; // class <%parserclass%>

/***** verbatim namespace.bottom ********/
## for v in verbatim.namespace_bottom
<% v %>
## endfor
/***** verbatim namespace.bottom ********/

} // namespace <%namespace%>

/***** verbatim file.bottom ********/
## for v in verbatim.file_bottom
<% v %>
## endfor
/***** verbatim file.bottom ********/

)DELIM"s;



} // namespace yalr::algo::slr