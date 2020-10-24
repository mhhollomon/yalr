#include "codegen/fsm_builders.hpp"

namespace yalr::codegen {

nfa_machine &nfa_machine::union_in(const nfa_machine &o) {

    // Renumber the states in the `o` machine as we add them.
    // This will hold a renumbering scheme.
    std::map<nfa_state_id, nfa_state_id>renumber_map;

    // add the states with new state ids
    for( auto &[id, state] : o.states_) {
        nfa_state_id new_state_id = nfa_state_id::get_next_id();
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

    // hook the start state of `o` to our start state with
    // an epsilon.
    auto &our_start_state = states_.at(start_state_);
    
    our_start_state.transitions_.emplace(std::monostate{}, renumber_map.at(o.start_state_));

    return *this;
}

nfa_machine &nfa_machine::concat_in(const nfa_machine &o) {
    return *this;
}

nfa_machine &nfa_machine::close_in(const nfa_machine &o) {
    return *this;
}

std::unique_ptr<nfa_machine> nfa_machine::build_from_string(std::string_view input, 
        symbol_identifier_t sym_id) {

    auto retval = std::make_unique<nfa_machine>();

    auto current_state_id = nfa_state_id::get_next_id();

    auto [ iter, _ ] = retval->states_.emplace(current_state_id, nfa_state{});

    auto * new_state = &(iter->second);

    retval->start_state_ = current_state_id;

    for (char c : input) {
        current_state_id = nfa_state_id::get_next_id();
        
        new_state->transitions_.emplace(c, current_state_id);

        auto [iter2, _] = retval->states_.emplace(current_state_id, nfa_state{});
        new_state = &(iter2->second);
    }

    retval->accepting_states_.insert(current_state_id);
    new_state->accepting = true;
    new_state->accepted_symbol = sym_id;

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
