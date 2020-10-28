#include "codegen/fsm_builders.hpp"

namespace yalr::codegen {

nfa_machine::nfa_machine(char c) : partial_(true) {
    auto start_state_id = nfa_state_identifier_t::get_next_id();

    auto [start_state_iter, _] = states_.emplace(start_state_id, start_state_id);

    start_state_ = start_state_id;

    auto final_state_id = nfa_state_identifier_t::get_next_id();

    start_state_iter->second.add_transition(c, final_state_id);

    auto [final_state, __] = states_.emplace(final_state_id, final_state_id);
    final_state->second.accepting_ = true;

    accepting_states_.insert(final_state_id);
}

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

nfa_machine &nfa_machine::close_in() {
    auto &new_final = append_epsilon_state();
    new_final.add_epsilon_transition(start_state_);

    auto new_start_state_id = nfa_state_identifier_t::get_next_id();
    auto new_start_state = nfa_state{new_start_state_id};

    new_start_state.add_epsilon_transition(start_state_);
    new_start_state.add_epsilon_transition(new_final.id_);
    start_state_ = new_start_state_id;
    return *this;
}

nfa_machine &nfa_machine::plus_in() {
    auto &new_final = append_epsilon_state();
    new_final.accepting_ = true;
    new_final.add_epsilon_transition(start_state_);
    accepting_states_.insert(new_final.id_);

    return *this;
}

nfa_machine &nfa_machine::option_in() {
    auto &new_final = append_epsilon_state();
    accepting_states_.insert(new_final.id_);
    new_final.accepting_ = true;
    auto new_start_state_id = nfa_state_identifier_t::get_next_id();
    auto new_start_state = nfa_state{new_start_state_id};

    new_start_state.add_epsilon_transition(start_state_);
    new_start_state.add_epsilon_transition(new_final.id_);
    start_state_ = new_start_state_id;
    states_.emplace(new_start_state_id, new_start_state);

    return *this;
}

nfa_machine &nfa_machine::add_transition(nfa_state_identifier_t from_state_id, 
        nfa_state_identifier_t to_state_id, char c) {

    auto &from_state = states_.at(from_state_id);

    from_state.add_transition(c, to_state_id);

    return *this;
}

nfa_machine &nfa_machine::add_epsilon_transition(nfa_state_identifier_t from_state_id, 
        nfa_state_identifier_t to_state_id) {

    auto &from_state = states_.at(from_state_id);

    from_state.add_epsilon_transition(to_state_id);

    return *this;
}

nfa_state &nfa_machine::append_epsilon_state() {
    if (accepting_states_.size() > 1) {
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
    } else {
        auto iter = accepting_states_.begin();
        auto &final_state = states_.at(*iter);
        accepting_states_.clear();
        final_state.accepting_ = false;
        return final_state;
    }
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


/*--------------------------*/
} // namespace yalr::codegen
