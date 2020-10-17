#pragma once

#include <string>

namespace yalr::algo::slr {

using namespace std::string_literals;

const std::string slr_parser_template =
R"DELIM(

namespace <%namespace%> {  // for the parser

template<typename LexerClass>
class <%parserclass%> {
public:
    using lexer_class = LexerClass;

private:
    lexer_class& lexer;
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
    <%parserclass%>(lexer_class& l) : lexer(l){};

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

/***** verbatim parser.bottom start ********/
## for v in verbatim.parser_bottom
<% v %>
## endfor
/***** verbatim parser.bottom end ********/

}; // class <%parserclass%>


} // namespace <%namespace%>

)DELIM"s;


} // namespace yalr::algo::slr
