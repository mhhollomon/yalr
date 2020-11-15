#include "codegen/fsm_builders.hpp"

#include "codegen/dice_trans.hpp"
#include <algorithm>

namespace yalr::codegen {

struct nfa_builder {


    std::map<char, char> const escape_map = {
        { 'f', '\f' },
        { 'n', '\n' },
        { 'r', '\r' },
        { 't', '\t' },
        { 'v', '\v' },
        { '0', '\0' },
    };

std::map<char, std::set<input_symbol_t>> const class_escape_map = {
    {'d', { input_symbol_t{sym_char, '0', '9'}, } },
    {'D', { input_symbol_t{sym_char, '\0', '0'-1, },
              input_symbol_t{sym_char, '9'+1, '\x7F'}, }, },

    {'s', { input_symbol_t{sym_char, '\t', '\r'},
              input_symbol_t{' '}, }, },
    {'S', { input_symbol_t{sym_char, '\0', '\t'-1},
              input_symbol_t{sym_char, '\r'+1, ' '-1},
              input_symbol_t{sym_char, ' '+1, '\x7F'}, }, },

    {'w', { input_symbol_t{sym_char, '0', '9'},
              input_symbol_t{sym_char, 'A', 'Z' },
              input_symbol_t{sym_char, 'a', 'z' },
              input_symbol_t{'_'}, }, },
    {'W', { input_symbol_t{sym_char, '\0', '0'-1, },
              input_symbol_t{sym_char, '9'+1, 'A'-1 },
              input_symbol_t{sym_char, 'Z'+1, 'a'-1 },
              input_symbol_t{sym_char, 'z'+1, '_'-1},
              input_symbol_t{sym_char, '_'+1, '\x7f' }, }, },

};


std::set<input_symbol_t> const dot_ranges = {
    { input_symbol_t{sym_char, '\0', '\n'-1},
      input_symbol_t{sym_char, '\n'+1, '\x7F'}, }, 
};


    std::set<input_symbol_t> parse_escape_to_set() {

        if (*first_ != '\\') {
            std::cerr << "ERROR - expecting back-slash\n";
            return {};
        }
        ++first_;

        if (first_ == last_) {
            std::cerr << "ERROR- bare back-slash at end of input\n";
            return {};
        }

        char c = *first_;
        ++first_;

        auto map_iter = escape_map.find(c);
        if (map_iter != escape_map.end()) {
            c = map_iter->second;
            return { c };
        }

        auto set_iter = class_escape_map.find(c); 
        if (set_iter != class_escape_map.end()) {
            return set_iter->second;
        }



        return { input_symbol_t{c} };
    }

    std::unique_ptr<nfa_machine> parse_escape() {

        auto chars = parse_escape_to_set();

        std::unique_ptr<nfa_machine> retval;

        nfa_state_identifier_t start_state_id;
        nfa_state_identifier_t final_state_id;

        for ( auto c : chars ) {
            if (not retval) {
                retval = std::make_unique<nfa_machine>(c);
                start_state_id = retval->start_state_id_;
                final_state_id = *(retval->accepting_states_.begin());
            } else {
                retval->add_transition(c, start_state_id, final_state_id);
            }
        }

        return retval;

    }
    
    // ------------------------------------------------------
    // Parse a character class. Handle negation.

    std::unique_ptr<nfa_machine> handle_char_class() { 

        if (*first_ != '[') {
            std::cerr << "ERROR - expecting open bracket\n";
            return {};
        }

        ++first_;
        
        bool negated_class = false;
        
        if (*first_ == '^') {
            negated_class = true;
            ++first_;
        }
            

        std::set<input_symbol_t>class_members;
        bool done = false;
        bool close_seen = false;
        while (not done) {

            if (first_ == last_) {
                done = true;
                continue;
            }

            if (*first_ == ']') {
                done = true;
                close_seen = true;
                ++first_;
            } else if (*first_ == '\\') {

                auto escaped_chars = parse_escape_to_set();
                class_members.insert(escaped_chars.begin(), escaped_chars.end());
            } else {
                auto letter = *first_;
                ++first_;
                input_symbol_t symbol{letter};
                if (*first_ == '-') {
                    ++first_;
                    auto end_letter = *first_;
                    ++first_;

                    if (end_letter <= letter) {
                        std::cerr << "ERROR: character class - invalid range - end must be bigger that start\n";
                        return {};
                    }
                    symbol.last_ = end_letter;
                }

                class_members.emplace(std::move(symbol));
            }
        }

        if (not close_seen) {
            std::cerr << "ERROR: character class opened, but not close\n";
            return {};
        }
        if (class_members.empty()) {
            std::cerr << "ERROR: empty character class not supported\n";
            return {};
        }

        auto retval = std::make_unique<nfa_machine>(true);

        auto & start_state = retval->add_state();

        retval->start_state_id_ = start_state.id_;

        auto & new_state = retval->add_state();
        auto new_state_id = new_state.id_;
        new_state.accepting_ = true;
        for (auto c : class_members) {
            start_state.add_transition(c, new_state_id);
        }
        retval->accepting_states_.insert(new_state_id);
        
        // negate the class.
        // Use dice_trans to make sure there arre no overlaps.
        //
        if (negated_class) {
            std::set<diced_trans_t> wood_pile;
            for (auto iter = start_state.transitions_.begin(); iter != start_state.transitions_.end(); ) {
                auto const &[sym, to_id] = *iter;
                if (sym.sym_type_ == sym_epsilon) {
                    ++iter;
                    continue;
                }
                wood_pile.emplace(sym.first_, sym.last_, to_id);
                iter = start_state.transitions_.erase(iter);
            }
                
            auto results = dice_transitions(wood_pile);
            
            // Can't sort a set, so transfer to a vector.
            std::vector<diced_trans_t> new_trans{results.begin(), results.end()};
            
            // sort the return so that it is in char order. rather than
            // length order.
            std::sort(new_trans.begin(), new_trans.end(), 
                [](diced_trans_t const &l, diced_trans_t const &r) {
                    return (l.low < r.low or
                        (l.low == r.low and l.high < r.high));
                    }
                );
            
            char current_char = '\0';
            bool has_trailer = true;
            
            for (auto const &t : new_trans) {
                if (current_char < t.low) {
                    start_state.transitions_.emplace(input_symbol_t{t.sym_type, current_char, static_cast<char>(t.low-1)}, t.to_state_id);
                    if (t.high == '\x7f') {
                        has_trailer = false;
                    } else {
                        current_char = t.high+1;
                    }
                }
            }
            
            if (has_trailer) {
                start_state.transitions_.emplace(input_symbol_t{sym_char, current_char, '\x7f'}, new_state_id);
            }
        }

        return retval;

    }

    std::unique_ptr<nfa_machine> parse_paren() {
        std::unique_ptr<nfa_machine> retval;

        if (*first_ != '(') {
            std::cerr << "ERROR - not at open paren\n";
            return {};
        }

        ++first_;
        // we want to handle a "bare open" as if it were escaped
        if (first_ == last_  and level_ < 2) {
            return std::make_unique<nfa_machine>('(');
        }

        retval = parse_regex();

        if (first_ == last_) {
            std::cerr << "ERROR - unmatched open paren\n";
            return {};
        }

        if (*first_ != ')') {
            std::cerr << "ERROR - not at close paren\n";
            return {};
        }

        ++first_;

        return retval;
    }

    std::unique_ptr<nfa_machine> handle_dot() {
        std::unique_ptr<nfa_machine> retval;

        nfa_state_identifier_t start_state_id;
        nfa_state_identifier_t final_state_id;

        for ( auto c : dot_ranges ) {
            if (not retval) {
                retval = std::make_unique<nfa_machine>(c);
                start_state_id = retval->start_state_id_;
                final_state_id = *(retval->accepting_states_.begin());
            } else {
                retval->add_transition(c, start_state_id, final_state_id);
            }
        }

        return retval;
    }

    void handle_possible_modifier(nfa_machine& nfa) {
        if (first_ == last_) return ;

        char c = *first_;

        if (c == '*') {
            nfa.close_in();
        } else if (c == '+') {
            nfa.plus_in();
        } else if (c == '?') {
            nfa.option_in();
        } else {
            // not ours - return without incrementing.
            return;
        }

        ++first_;
    }

    std::unique_ptr<nfa_machine> parse_alternate() {
        std::unique_ptr<nfa_machine> retval;

        while (first_ != last_) {
            std::unique_ptr<nfa_machine> sub_mach_ptr = nullptr;
            char c = *first_;

            // hit a stop marker
            if ((c == ')' and level_ > 1) or c == '|') {
                break;
            }

            switch (c) {
                case '(' :
                    sub_mach_ptr = parse_paren();
                    handle_possible_modifier(*sub_mach_ptr);
                    break;

                case '\\' :
                    sub_mach_ptr = parse_escape();
                    handle_possible_modifier(*sub_mach_ptr);
                    break;

                case '[' :
                    sub_mach_ptr = handle_char_class();
                    handle_possible_modifier(*sub_mach_ptr);
                    break;

                case '.' :
                    sub_mach_ptr = handle_dot();
                    ++first_;
                    handle_possible_modifier(*sub_mach_ptr);
                    break;

                default :
                    // This is a bit messy trying to be state efficient and
                    // only generate extra epsilon states if necessary.
                    // concat_char() doesn't create an epsilon transition at all.
                    //
                    // IF we have a bare character match, then
                    //      if we have no return value yet, make this char the
                    //      return value.
                    //      Otherwise, use concat_char to tag it on the end.
                    // However, if we have a modifier, then we have to go whole hog
                    // and create a machine and use concat_in() instead.
                    // 
                    ++first_;
                    if (*first_ == '*' or *first_ == '?' or *first_ == '+') {
                        sub_mach_ptr = std::make_unique<nfa_machine>(c);
                        handle_possible_modifier(*sub_mach_ptr);
                    } else if (retval) {
                        retval->concat_char(c);
                    } else {
                        retval = std::make_unique<nfa_machine>(c);
                    }
                    break;
            }

            if (sub_mach_ptr) {
                if (not retval) {
                    retval = std::move(sub_mach_ptr);
                } else {
                    retval->concat_in(*sub_mach_ptr);
                }
            }

        }

        if(!retval) {
            // empty alternate. Create an epsilon machine
            retval = std::make_unique<nfa_machine>(true);
            auto start_state_id = nfa_state_identifier_t::get_next_id();
            auto end_state_id = nfa_state_identifier_t::get_next_id();

            retval->states_.emplace(start_state_id, nfa_state{start_state_id});
            auto const [end_state_iter, _] = retval->states_.emplace(end_state_id, nfa_state{end_state_id});
            retval->add_epsilon_transition(start_state_id, end_state_id);
            retval->start_state_id_ = start_state_id;
            end_state_iter->second.accepting_ = true;
            retval->accepting_states_.insert(end_state_id);

        }

        return retval;
    }

    // --------------------------------------------------------
    // Top level parse routine
    //
    std::unique_ptr<nfa_machine> parse_regex() {

        level_ += 1;
        std::vector<std::unique_ptr<nfa_machine>> machines;

        std::unique_ptr<nfa_machine> sub_mach_ptr;
        while (true) {
            sub_mach_ptr = parse_alternate();
            machines.emplace_back(std::move(sub_mach_ptr));
            if (first_ != last_) {
                if (*first_ == '|') {
                    ++first_;
                } else if (*first_ == ')')  {
                    if (level_ < 2) {
                        std::cerr << "ERROR - bare close parens\n";
                        return {};
                    }
                    break;
                } else {
                    std::cerr << "ERROR - expecting bar got '" << *first_ << "'\n";
                    std::cerr << "ERROR - internal error stopped before end\n";
                    return {};
                }
            } else {
                // end it
                break;
            }
        }

        std::unique_ptr<nfa_machine> retval;
        if (machines.size() == 1) {
            //retval = std::make_unique<nfa_machine>(machines.begin()->release());
            retval.reset(machines.begin()->release());
        } else {
            retval = std::make_unique<nfa_machine>(true);

            auto start_state_id =  nfa_state_identifier_t::get_next_id();
            auto [ iter, _] = retval->states_.emplace(start_state_id, nfa_state(start_state_id));
            auto & start_state = iter->second;
            retval->start_state_id_ = start_state_id;

            for ( auto &m : machines) {
                auto &copied_start = retval->copy_in(*m);
                start_state.add_epsilon_transition(copied_start.id_);
            }
        }

        level_ -= 1;
        return retval;

    }

    std::string_view::iterator first_;
    std::string_view::const_iterator last_;
    bool success = false;
    int level_ = 0;


    nfa_builder(std::string_view::iterator f, 
            std::string_view::const_iterator l) : first_(f), last_(l) {}



}; // nfa_builder


// 
// note that these are in this file so they can reference nfa_builder class
// without having to actually define it in the header
//
//
// Build from a simple string - the string matches literally.
// "a*[x]+" matches exactly the character sequence 'a', '*', '[' 'x' ']', '+'
//
// BUt we do have to deal with the two esacpe sequences \\ and \'
//
std::unique_ptr<nfa_machine> nfa_machine::build_from_string(std::string_view input, 
        symbol_identifier_t sym_id) {

    //std::cout << "build nfa from string '" << input << "' (" << sym_id << ")\n";

    std::unique_ptr<nfa_machine> retval;

    auto add_char = [&retval](char x) -> void {
        if (retval) {
            retval->concat_char(x);
        } else {
            retval = std::make_unique<nfa_machine>(x);
        }
    };

    bool in_esc = false;
    for (auto c : input) {
        if (in_esc) {
            if (c == '\\' or c == '\'') {
                add_char(c);
            } else {
                add_char('\\');
                add_char(c);
            }
            in_esc = false;
        } else if (c == '\\') {
            in_esc = true;
        } else {
            add_char(c);
        }
    }

    retval->partial_ = false;

    // should only be one state. but meh.
    for (auto state_id : retval->accepting_states_) {
        auto &state = retval->states_.at(state_id);
        state.accepted_symbol_ = sym_id;
    }


    return retval;
}

//
// Build for a regular expression with much of the trappings
//
std::unique_ptr<nfa_machine> nfa_machine::build_from_regex(std::string_view input, 
        symbol_identifier_t sym_id) {

    //std::cout << "build nfa from regex " << input << " (" << sym_id << ")\n";
    auto first = input.begin();
    auto last    = input.end();

    auto builder = nfa_builder{first, last};

    auto retval = builder.parse_regex();

    retval->partial_ = false;

    for (auto state_id : retval->accepting_states_) {
        auto &state = retval->states_.at(state_id);
        state.accepted_symbol_ = sym_id;
    }


    return retval;
}


std::unique_ptr<nfa_machine> build_nfa(const symbol_table &sym_tab) {

    
    std::string_view pattern;
    pattern_type pat_type;

    std::vector<std::unique_ptr<nfa_machine>> machines;

    for(auto const &[id, x] : sym_tab) {
        if (x.isterm()) {
            auto *data = x.get_data<symbol_type::terminal>();

            if (data->pat_type == pattern_type::ecma) continue;
            // eoi
            if (data->pattern.empty()) continue;

            pattern = data->pattern;
            pat_type = data->pat_type;

        } else if (x.isskip()) {
            auto *data = x.get_data<symbol_type::skip>();

            if (data->pat_type == pattern_type::ecma) continue;

            pattern = data->pattern;
            pat_type = data->pat_type;
        } else {
            continue;
        }

        if (pat_type == pattern_type::string) {
            machines.emplace_back(nfa_machine::build_from_string(pattern, id));
        } else {
            machines.emplace_back(nfa_machine::build_from_regex(pattern, id));
        }

    }

    auto retval = std::make_unique<nfa_machine>(false);

    auto start_state_id =  nfa_state_identifier_t::get_next_id();
    auto [ iter, _] = retval->states_.emplace(start_state_id, nfa_state(start_state_id));
    auto & start_state = iter->second;
    retval->start_state_id_ = start_state_id;

    for ( auto &m : machines) {
        auto &copied_start = retval->copy_in(*m);
        start_state.add_epsilon_transition(copied_start.id_);
    }

    return retval;
}


} // yalr::codegen
