#include "codegen/fsm_builders.hpp"

namespace yalr::codegen {


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


template< typename T>
std::set<char> parse_escape_to_set(T &first, const T& last) {
    if (first == last) {
        std::cerr << "ERROR: bare back-slash at end of input\n";
        return {};
    }

    char c = *first;
    ++first;

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

template< typename T>
std::unique_ptr<nfa_machine> parse_escape(T &first, const T& last) {

    auto chars = parse_escape_to_set(first, last);

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

template< typename T>
std::unique_ptr<nfa_machine> handle_char_class(T &first, const T& last ) { 
    std::set<char>class_members;
    bool done = false;
    bool close_seen = false;
    while (not done) {

        if (first == last) {
            done = true;
            continue;
        }

        if (*first == ']') {
            done = true;
            close_seen = true;
        } else if (*first == '\\') {
            ++first;
            if (first == last) {
                done = true;
                continue;
            } else {
                char c = *first;
                auto map_iter = escape_map.find(c);
                if (map_iter != escape_map.end()) {
                    c = map_iter->second;
                }
                class_members.insert(c);
            }
        } else {
            class_members.insert(*first);
        }
        ++first;
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

template<typename T>
void handle_possible_modifier(T &first, const T& last, nfa_machine& nfa) {
    if (first == last) return ;

    char c = *first;

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

    ++first;
}

template<typename T>
std::unique_ptr<nfa_machine> parse_string(T &first, const T& last) {
    std::unique_ptr<nfa_machine> retval;

    std::unique_ptr<nfa_machine> sub_mach_ptr;
    while (first != last) {
        char c = *first;
        ++first;
        if (c == '\\') {
            sub_mach_ptr = parse_escape(first, last);
        } else if (c == '[') {
            sub_mach_ptr = handle_char_class(first, last);
        } else {
            sub_mach_ptr = std::make_unique<nfa_machine>(c);
        }
        handle_possible_modifier(first, last, *sub_mach_ptr);
        if (not retval) {
            retval = std::move(sub_mach_ptr);
        } else {
            retval->concat_in(*sub_mach_ptr);
        }

    }

    return retval;
}

std::unique_ptr<nfa_machine> nfa_machine::build_from_string(std::string_view input, 
        symbol_identifier_t sym_id) {

    auto current = input.begin();
    auto last    = input.end();

    auto retval = parse_string(current, last);

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
