#include "codegen/fsm_builders.hpp"

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

std::map<char, std::set<char>> const class_escape_map = {
    {'d', { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9' } },
    //{'s', { '\t', '\v', '\f', ' ', '\xa0', '\n', '\r' } },
    {'s', { '\t', '\v', '\f', ' ', '\n', '\r' } },
    // This is really isn't sustainable for the negated classes and "large"
    // class (such as \w)
};



    std::set<char> parse_escape_to_set() {

        if (*first_ != '\\') {
            std::cerr << "ERROR - expecting back-slash\n";
            return {};
        }
        ++first_;

        if (first_ == last_) {
            std::cerr << "ERROR- bare balck-slash at end of input\n";
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



        return { c };
    }

    std::unique_ptr<nfa_machine> parse_escape() {

        auto chars = parse_escape_to_set();

        std::unique_ptr<nfa_machine> retval;

        nfa_state_identifier_t start_state_id;
        nfa_state_identifier_t final_state_id;

        for ( auto c : chars ) {
            if (not retval) {
                retval = std::make_unique<nfa_machine>(c);
                start_state_id = retval->start_state_;
                final_state_id = *(retval->accepting_states_.begin());
            } else {
                retval->add_transition(start_state_id, final_state_id, c);
            }
        }

        return retval;

    }

    std::unique_ptr<nfa_machine> handle_char_class() { 

        if (*first_ != '[') {
            std::cerr << "ERROR - expecting open bracket\n";
            return {};
        }

        ++first_;

        std::set<char>class_members;
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
            } else if (*first_ == '\\') {
                auto escaped_chars = parse_escape_to_set();
                class_members.insert(escaped_chars.begin(), escaped_chars.end());
            } else {
                class_members.insert(*first_);
                ++first_;
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

        auto start_state_id = nfa_state_identifier_t::get_next_id();
        auto start_state = nfa_state{start_state_id};

        retval->start_state_ = start_state_id;

        auto new_state_id = nfa_state_identifier_t::get_next_id();
        auto new_state = nfa_state{new_state_id};
        new_state.accepting_ = true;
        for (char c : class_members) {
            start_state.add_transition(c, new_state_id);
        }
        retval->accepting_states_.insert(new_state_id);
        retval->states_.emplace(new_state_id, std::move(new_state));
        retval->states_.emplace(start_state_id, std::move(start_state));


        return retval;

    }

    std::unique_ptr<nfa_machine> parse_paren() {
        std::unique_ptr<nfa_machine> retval;

        if (*first_ != '(') {
            std::cerr << "ERROR - not at open paren\n";
            return {};
        }

        ++first_;
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

        std::unique_ptr<nfa_machine> sub_mach_ptr;
        while (first_ != last_) {
            char c = *first_;

            // hit a stop marker
            if (c == ')' or c == '|') {
                break;
            }

            switch (c) {
                case '(' :
                    sub_mach_ptr = parse_paren();
                    break;

                case '\\' :
                    sub_mach_ptr = parse_escape();
                    break;

                case '[' :
                    sub_mach_ptr = handle_char_class();
                    break;

                default :
                    ++first_;
                    sub_mach_ptr = std::make_unique<nfa_machine>(c);
                    break;
            }

            handle_possible_modifier(*sub_mach_ptr);
            if (not retval) {
                retval = std::move(sub_mach_ptr);
            } else {
                retval->concat_in(*sub_mach_ptr);
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
            retval->start_state_ = start_state_id;
            end_state_iter->second.accepting_ = true;
            retval->accepting_states_.insert(end_state_id);

        }

        return retval;
    }


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

        auto retval = std::make_unique<nfa_machine>(true);

        auto start_state_id =  nfa_state_identifier_t::get_next_id();
        auto [ iter, _] = retval->states_.emplace(start_state_id, nfa_state(start_state_id));
        auto & start_state = iter->second;
        retval->start_state_ = start_state_id;

        for ( auto &m : machines) {
            auto &copied_start = retval->copy_in(*m);
            start_state.add_epsilon_transition(copied_start.id_);
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


// note that this is in this file so it can reference nfa_builder class
// without have to actually define it in the header
//
std::unique_ptr<nfa_machine> nfa_machine::build_from_string(std::string_view input, 
        symbol_identifier_t sym_id) {

    std::cout << "build nfa from string " << input << "\n";
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

    std::vector<std::unique_ptr<nfa_machine>> machines;

    for(auto const &[id, x] : sym_tab) {
        if (x.isterm()) {
            auto *data = x.get_data<symbol_type::terminal>();

            if (data->pat_type != pattern_type::string) continue;

            pattern = data->pattern;

        } else if (x.isskip()) {
            auto *data = x.get_data<symbol_type::skip>();

            if (data->pat_type != pattern_type::string) continue;

            pattern = data->pattern;
        } else {
            continue;
        }

        machines.emplace_back(nfa_machine::build_from_string(pattern, id));

    }

    auto retval = std::make_unique<nfa_machine>(false);

    auto start_state_id =  nfa_state_identifier_t::get_next_id();
    auto [ iter, _] = retval->states_.emplace(start_state_id, nfa_state(start_state_id));
    auto & start_state = iter->second;
    retval->start_state_ = start_state_id;

    for ( auto &m : machines) {
        auto &copied_start = retval->copy_in(*m);
        start_state.add_epsilon_transition(copied_start.id_);
    }

    return retval;
}


} // yalr::codegen
