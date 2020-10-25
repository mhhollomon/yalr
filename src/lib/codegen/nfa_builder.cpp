#include "codegen/fsm_builders.hpp"

namespace yalr::codegen {

nfa_machine &nfa_machine::union_in(const nfa_machine &o) {

    auto &their_start_state = copy_in(o);
    // hook the start state of `o` to our start state with
    // an epsilon.
    auto &our_start_state = states_.at(start_state_);
    
    our_start_state.add_epsilon_transition(their_start_state.id_);

    return *this;
}

// Copy the states of another machine into this one - renumbering
// in order make sure that no collisions happen.
// Also adds the accepting states to this one.
// Returns our copy of the start state of the other machine.
//
nfa_state & nfa_machine::copy_in(const nfa_machine &o) {
    // Renumber the states in the `o` machine as we add them.
    // This will hold a renumbering scheme.
    std::map<nfa_state_identifier_t, nfa_state_identifier_t>renumber_map;

    // add the states with new state ids
    // not `&` as we want a fresh copy of the state.
    for( auto [id, state] : o.states_) {
        auto new_state_id = nfa_state_identifier_t::get_next_id();
        state.id_ = new_state_id;
        renumber_map.emplace(id, new_state_id);
        states_.emplace(new_state_id, state);
    }

    // using the renumber_map as a guide, walk through
    // the new states and update the transitions.
    for( auto &[old_id, new_id] : renumber_map) {
        auto &current_state = states_.at(new_id);
        
        auto old_transitions = current_state.transitions_;
        current_state.transitions_.clear();
        for (auto &[sym_variant, trans_state_id] : old_transitions) {
            current_state.transitions_.emplace(sym_variant,
                    renumber_map.at(trans_state_id));
        }
    }

    // Add their accepting states to ours - while renumbering.
    for ( auto old_acc_id : o.accepting_states_) {
        accepting_states_.insert(renumber_map.at(old_acc_id));
    }

    return states_.at(renumber_map.at(o.start_state_));
}

nfa_machine &nfa_machine::concat_in(const nfa_machine &o) {

    if (states_.size() > 0 ) {
        auto &new_final = append_epsilon_state();
        auto &their_start_state = copy_in(o);

        new_final.add_epsilon_transition(their_start_state.id_);
    } else {
        auto &their_start_state = copy_in(o);
        start_state_ = their_start_state.id_;
    }

    return *this;
}

nfa_machine &nfa_machine::close_in(const nfa_machine &o) {
    return *this;
}

nfa_machine &nfa_machine::add_transition(nfa_state_identifier_t from_state_id, 
        nfa_state_identifier_t to_state_id, char c) {

    auto from_state = states_.at(from_state_id);

    from_state.add_transition(c, to_state_id);

    return *this;
}

nfa_machine &nfa_machine::add_epsilon_transition(nfa_state_identifier_t from_state_id, 
        nfa_state_identifier_t to_state_id) {

    auto from_state = states_.at(from_state_id);

    from_state.add_epsilon_transition(to_state_id);

    return *this;
}

nfa_state &nfa_machine::append_epsilon_state() {
    auto new_state_id = nfa_state_identifier_t::get_next_id();
    auto new_state = nfa_state{new_state_id};
    for( auto final_state_id : accepting_states_ ) {
        auto &final_state = states_.at(final_state_id);
        if (partial_) {
            final_state.accepting_ = false;
        }
        final_state.add_epsilon_transition(new_state_id);
    }
    auto [iter, _] = states_.emplace(new_state_id, std::move(new_state));
    accepting_states_.clear();

    return iter->second;
}

nfa_machine &nfa_machine::concat_char(char c) {
    auto new_state_id = nfa_state_identifier_t::get_next_id();
    auto new_state = nfa_state{new_state_id};

    if (states_.size() == 0) {
        // starting fresh
        start_state_ = new_state_id;
    } else {
        // find all the accepting states of the base
        // and add a transition to the new state on epsilon.
        for( auto final_state_id : accepting_states_ ) {
            auto &final_state = states_.at(final_state_id);
            if (partial_) {
                final_state.accepting_ = false;
            }
            final_state.add_epsilon_transition(new_state_id);
        }
        if (partial_) {
            accepting_states_.clear();
        }
    }
    auto [iter, _] = states_.emplace(new_state_id, std::move(new_state));

    auto accepting_state_id = nfa_state_identifier_t::get_next_id();

    iter->second.add_transition(c, accepting_state_id);
    accepting_states_.insert(accepting_state_id);

    new_state = nfa_state{accepting_state_id};
    new_state.accepting_ = true;
    states_.emplace(accepting_state_id, new_state);

    return *this;
}

std::map<char, char> const escape_map = {
    { 'n', '\n' },
    { 't', '\t' },
    { 'r', '\r' },
};


template< typename T>
void handle_escape_char(T &first, const T& last, nfa_machine& nfa) {
    if (first == last) {
        std::cerr << "ERROR: bare back-slash at end of end put\n";
        return;
    }

    char c = *first;
    ++first;

    auto map_iter = escape_map.find(c);
    if (map_iter != escape_map.end()) {
        c = map_iter->second;
    }

    nfa.concat_char(c);

}

template< typename T>
std::unique_ptr<nfa_machine> handle_char_class(T &first, const T& last, nfa_machine& nfa) {
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
void handle_next_char(T &first, const T& last, nfa_machine& nfa) {
    bool in_slash = false;
    while (first != last) {
        if (in_slash) {
            char c = *first;
            ++first;
            auto map_iter = escape_map.find(c);
            if (map_iter != escape_map.end()) {
                c = map_iter->second;
            }
            nfa.concat_char(c);
            in_slash = false;
        } else if (*first == '\\') {
            ++first;
            in_slash = true;
        } else if (*first == '[') {
            ++first;
            auto mach = handle_char_class(first, last, nfa);
            nfa.concat_in(*mach);
        } else {
            char c = *first;
            ++first;
            nfa.concat_char(c);
        }

    }
}

std::unique_ptr<nfa_machine> nfa_machine::build_from_string(std::string_view input, 
        symbol_identifier_t sym_id) {

    auto retval = std::make_unique<nfa_machine>(true);

    auto current = input.begin();
    auto last    = input.end();

    handle_next_char(current, last, *retval);

    retval->partial_ = false;

    for (auto state_id : retval->accepting_states_) {
        auto &state = retval->states_.at(state_id);
        state.accepted_symbol_ = sym_id;
    }




    return retval;
}

std::unique_ptr<nfa_machine> build_nfa(const symbol_table &sym_tab) {
    std::unique_ptr<nfa_machine> retval;

    for(auto const &[id, x] : sym_tab) {
        if (not x.isterm()) continue;

        auto *data = x.get_data<symbol_type::terminal>();

        if (data->pat_type != pattern_type::string) continue;

        auto pattern = data->pattern;

        auto temp = nfa_machine::build_from_string(pattern, id);

        if (not retval) {
            retval = std::make_unique<nfa_machine>(*temp.release());
        } else {
            retval->union_in(*temp);
        }
    }

    return retval;
}

} // yalr::codegen
