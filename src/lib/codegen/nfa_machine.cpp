#include "codegen/fsm_builders.hpp"
#include <deque>

namespace yalr::codegen {

input_symbol_t epsilon_symbol = { sym_epsilon, 0, 0 };

nfa_state & nfa_machine::add_state() {
    auto new_state_id = nfa_state_identifier_t::get_next_id();

    auto [ iter, _ ] = states_.emplace(new_state_id, new_state_id);

    return iter->second;

}

nfa_machine::nfa_machine(input_symbol_t sym) : partial_(true) {
    auto & start_state = add_state();

    start_state_id_ = start_state.id_;

    auto & final_state = add_state();

    auto final_state_id = final_state.id_;

    start_state.add_transition(sym, final_state_id);

    final_state.accepting_ = true;

    accepting_states_.insert(final_state_id);
}



/*-----------------------------------------------------*/
// union_in
//
// Change the base machine to compute base|o
//
nfa_machine &nfa_machine::union_in(const nfa_machine &o) {

    auto &their_start_state = copy_in(o);

    auto &new_start_state = add_state();

    new_start_state.add_epsilon_transition(start_state_id_);
    new_start_state.add_epsilon_transition(their_start_state.id_);

    start_state_id_ = new_start_state.id_;

    return *this;
}

/*-----------------------------------------------------*/
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

    return states_.at(renumber_map.at(o.start_state_id_));
}

/*-----------------------------------------------------*/
// Concatentation.
// hook all our accepting states to the copied in start state.
// This only really works if things are partials.
// 
nfa_machine &nfa_machine::concat_in(const nfa_machine &o) {

    if (states_.size() > 0 ) {
        // grab a copy of the set of our current final states then clear it.
        //
        auto our_old_finals = accepting_states_;
        accepting_states_.clear();

        auto &their_start_state = copy_in(o);

        if (our_old_finals.size() == 1 and 
                states_.at(*(our_old_finals.begin())).transitions_.size() == 0) {
            // WE only have one final state, and it has no outgoing transitions
            // so lets just remove it and hook into the start state
            // of the other machine.
            // This would be possible to do even for multiple
            // final states but the code is a bit ugly.
            //
            // But for simple cases - such as 'abc', it saves an epsilon
            // per concatenation.
            //
            auto final_state_id = *(our_old_finals.begin());

            for (auto &[id, state] : states_) {
                for (auto iter = state.transitions_.begin() ; iter != state.transitions_.end(); ){
                    if (iter->second == final_state_id) {
                        state.add_transition(iter->first, their_start_state.id_);
                        iter = state.transitions_.erase(iter);
                    } else {
                        ++iter;
                    }
                }
            }

            states_.erase(final_state_id);
        } else {
            // Hook all our old final states up to the 
            // start state of the other machine.
            for (auto id : our_old_finals) {
                auto & state = states_.at(id);
                state.accepting_ = false;
                state.add_epsilon_transition(their_start_state.id_);
            }
        }
    } else {
        // We are a new empty machine, so just copy in the
        // other machine.
        //
        auto &their_start_state = copy_in(o);
        start_state_id_ = their_start_state.id_;
    }

    return *this;
}

/*-----------------------------------------------------*/
// Kleene star
// Wrap all the accepting states back to the start.
// Add a new start that is also an accepting state (for the zero case)
//
nfa_machine &nfa_machine::close_in() {
    for (auto id : accepting_states_ ) {
        states_.at(id).add_epsilon_transition(start_state_id_);
    }

    auto &new_start_state = add_state();
    new_start_state.accepting_ = true;

    new_start_state.add_epsilon_transition(start_state_id_);
    start_state_id_ = new_start_state.id_;
    accepting_states_.insert(start_state_id_);

    return *this;
}

/*-----------------------------------------------------*/
// '+' operator (1 or more)
// Like '*' but we don't add the epsilon case.
//
nfa_machine &nfa_machine::plus_in() {
    for (auto id : accepting_states_ ) {
        states_.at(id).add_epsilon_transition(start_state_id_);
    }

    return *this;
}

/*-----------------------------------------------------*/
// '?' optional (0 or 1)
//
nfa_machine &nfa_machine::option_in() {
    auto &new_start_state = add_state();
    new_start_state.accepting_ = true;

    new_start_state.add_epsilon_transition(start_state_id_);
    start_state_id_ = new_start_state.id_;
    accepting_states_.insert(start_state_id_);
    return *this;
}

/*-----------------------------------------------------*/
nfa_machine & nfa_machine::add_transition(input_symbol_t symb, nfa_state_identifier_t from_state_id, 
        nfa_state_identifier_t to_state_id) {

    auto &from_state = states_.at(from_state_id);

    from_state.add_transition(symb, to_state_id);

    return *this;
}

/*-----------------------------------------------------*/
nfa_machine &nfa_machine::add_transition(char c, nfa_state_identifier_t from_state_id, 
        nfa_state_identifier_t to_state_id) {

    auto &from_state = states_.at(from_state_id);

    from_state.add_transition(c, to_state_id);

    return *this;
}

/*-----------------------------------------------------*/
nfa_machine &nfa_machine::add_epsilon_transition(nfa_state_identifier_t from_state_id, 
        nfa_state_identifier_t to_state_id) {

    auto &from_state = states_.at(from_state_id);

    from_state.add_epsilon_transition(to_state_id);

    return *this;
}

/*-----------------------------------------------------*/
// update the base machine so it matches 'c' after doing
// whatever it currently does.
//
// In all cases, we can do that by adding a char transition
// from all the final states to a new final state.
//
// This means we only add a single new state and no epsilons.
//
nfa_machine &nfa_machine::concat_char(char c) {
    auto & new_state = add_state();
    auto new_state_id = new_state.id_;

    if (states_.size() == 1) {
        // starting fresh
        start_state_id_ = new_state_id;
        auto & new_final_state = add_state();
        new_final_state.accepting_ = true;

        new_state.add_transition(c, new_final_state.id_);
        accepting_states_.insert(new_final_state.id_);

    } else {
        // find all the accepting states of the base
        // and add a transition to the new state on 'c'.
        for( auto final_state_id : accepting_states_ ) {
            auto &final_state = states_.at(final_state_id);
            if (partial_) {
                final_state.accepting_ = false;
            }
            final_state.add_transition(c, new_state_id);
        }
        if (partial_) {
            accepting_states_.clear();
        }
        accepting_states_.insert(new_state_id);
        new_state.accepting_ = true;
    }

    return *this;
}

/******************************************************/

void close_over_epsilon(nfa_machine &nfa, std::set<nfa_state_identifier_t> &cs) {
    std::deque<nfa_state_identifier_t> queue;
    std::set<nfa_state_identifier_t> seen;

    queue.insert(queue.cend(), cs.begin(), cs.end());
    while(not queue.empty()) {
        auto this_state_id = queue.back();
        queue.pop_back();

        //std::cout << "epsilon processing state = " << this_state_id << "\n";

        if (seen.count(this_state_id) > 0) { continue; }
        //std::cout << "epsilon not seen state = " << this_state_id << "\n";

        seen.insert(this_state_id);

        auto const & this_state = nfa.states_.at(this_state_id);

        auto [lower, upper] = this_state.transitions_.equal_range(epsilon_symbol);
        for(auto iter = lower; iter != upper; ++iter) {
            //std::cout << "epsilon adding " << iter->second << " to queue\n";
            cs.insert(iter->second);
            queue.emplace_front(iter->second);
        }

    }

    //std::cout << "epsilon queue length = " << queue.size() << "\n";
}

std::set<nfa_state_identifier_t> compute_new_state(nfa_machine &nfa, std::set<nfa_state_identifier_t> current_state, char input) {
    std::set<nfa_state_identifier_t> retval;

    for (auto this_state_id : current_state) {
        if (retval.count(this_state_id) > 0) { continue; }
        auto const &this_state = nfa.states_.at(this_state_id);

        for (auto const &[symb, state_id] : this_state.transitions_) {
            if (symb.sym_type_ == sym_epsilon) { continue; }
            if (retval.count(state_id) > 0) { continue; }
            if (input >= symb.first_ and input <= symb.last_) {
                retval.insert(state_id);
            }
        }
    }

    return retval;
        

}

bool is_accepting_state(nfa_machine &nfa, std::set<nfa_state_identifier_t> state_set) {
    for (auto x : nfa.accepting_states_) {
        if (state_set.count(x) > 0) {
            std::cout << "      Yes because of " << x << "\n";
            return true;
        }
    }

    std::cout << "      No\n";
    return false;
}

std::ostream &print_state_set(std::ostream &strm, std::set<nfa_state_identifier_t> state_set) {
    strm << " { ";
    for (auto id : state_set) {
        strm << int(id) << ", ";
    }
    strm << "}";

    return strm;
}

nfa_machine::run_results nfa_machine::run(std::string_view input) {
    int char_count = 0;
    auto current_iter = input.begin();
    std::set<nfa_state_identifier_t>current_state;

    current_state.insert(start_state_id_);
    close_over_epsilon(*this, current_state);

    std::cout << "=================== start ================\n";
    while (current_iter != input.end()) {
        std::cout << "state = "; print_state_set(std::cout, current_state) << "\n";
        auto new_state = compute_new_state(*this, current_state, *current_iter);
        close_over_epsilon(*this, new_state);
        if (new_state.empty()) {
            std::cout << "No transition for char = " << util::escape_char(*current_iter) << "\n";
            // failed to match the char
            std::cout << "  checking for acceptance in " ; print_state_set(std::cout, current_state) << "\n";
            if (is_accepting_state(*this, current_state)) {
                return { true, char_count };
            } else {
                return { false, char_count };
            }
        } else {
            std::cout << "found transition for char = " << util::escape_char(*current_iter) << "\n";
            ++current_iter;
            char_count += 1;
            current_state = new_state;
        }
    }

    std::cout << "ran out of input\n";
    std::cout << "  checking for acceptance in " ; print_state_set(std::cout, current_state) << "\n";
    if (is_accepting_state(*this, current_state)) {
        return { true, char_count };
    } else {
        return { false, char_count };
    }

}

bool nfa_machine::health_check(std::ostream &strm) {
    bool success = true;

    if (states_.count(start_state_id_) == 0) {
        strm << "ERROR  could not find start state with id = " << start_state_id_ << "\n";
        success = false;
    }

    if (accepting_states_.empty()) {
        strm << "ERROR  no accepting states on the list\n";
    }

    for ( auto sid : accepting_states_ ) {
        try {
            auto s = states_.at(sid);
            if (s.accepting_ == false) {
                strm << "ERROR  state " << sid << "in accepting list but not marked as accepting\n";
                success = false;
            }

        } catch ( std::out_of_range const &e) {
            strm << "ERROR  could not find state id = " << sid << "\n";
            success = false;
        }
    }

    for (auto const &[sid, s] : states_) {
        if (s.accepting_ == true) {
            if (accepting_states_.count(sid) == 0) {
                strm << "ERROR  state " << sid << " marked as accepting but not in accepting list\n";
                success = false;

            }
        } else if (s.transitions_.empty()) {
            strm << "ERROR  state " << sid << " is nonaccepting but has no out-bound transitions\n";
            success = false;
        }
    }

    return success;

}

void nfa_machine::dump(std::ostream &strm) {
    strm << "Start state = " << start_state_id_ << "\n";

    for (auto const &[sid, s] : states_) {
        strm << "state " << sid;
        if (s.accepting_) {
            strm << " (acc)";
        }
        strm << '\n';

        for ( auto const [ symb, new_state_id ] : s.transitions_) {
            strm << "    ";
            if (symb.sym_type_ == sym_epsilon) {
                strm << "<eps>";
            } else if (symb.sym_type_ == sym_char) {
                strm << util::escape_char(symb.first_);
                if (symb.last_ != symb.first_) {
                    strm << '-' << util::escape_char(symb.last_);
                }
            }
            strm << " --> " << new_state_id << "\n";
        }
    }
}


/*--------------------------*/
} // namespace yalr::codegen
