#include "codegen/fsm_builders.hpp"

#include <deque>

namespace yalr::codegen {

using nfa_state_set = std::set<nfa_state_id>;

using nfa_transition = std::map<char, nfa_state_set>;

// updates the given state to be the closure over epsilon
// in the given machine
void epsilon_close(nfa_state_set &states, const nfa_machine &nfa) { 

    std::deque<nfa_state_id>queue{states.begin(), states.end()};
    std::set<nfa_state_id>seen;

    while (not queue.empty()) {
        auto current_id = queue.back();
        queue.pop_back();
        if (seen.count(current_id) > 0)  {
            continue;
        }
        auto &current_state = nfa.states_.at(current_id);
        auto [lower, upper] = current_state.transitions_.equal_range(std::monostate{});
        for(auto iter = lower; iter != upper; ++iter) {
            states.insert(iter->second);
            if (seen.count(iter->second) == 0) {
                queue.push_front(iter->second);
            }
        }
    }
}


std::unique_ptr<dfa_machine> build_dfa(const nfa_machine &nfa) {

    // create the new start state
    nfa_state_set start_state;
    start_state.insert(nfa.start_state_);
    epsilon_close(start_state, nfa);

    std::map<nfa_state_set, nfa_transition> nfa_transitions;

    std::deque<nfa_state_set> queue;

    queue.push_front(start_state);

    while (not queue.empty()) {
        auto current_state_set = queue.back();
        queue.pop_back();

        /*
        std::cerr << "in the loop {";
        for (auto x : current_state_set) {
            std::cerr << x << ", ";
        }
        //std::cerr << "}\n";
        */

        if (nfa_transitions.count(current_state_set) > 0 ) continue;

        // build transitions for this state set
        nfa_transition nt;
        for (auto current_id : current_state_set) {
            auto current_state = nfa.states_.at(current_id);
            for (auto &[sym, new_state] : current_state.transitions_) {
                if (sym.index() == 0) continue;

                char c = std::get<1>(sym);
                auto iter = nt.find(c);
                if (iter == nt.end() ) {
                    auto new_set = nfa_state_set{};
                    new_set.insert(new_state);
                    nt.emplace(c, new_set);
                } else {
                    iter->second.insert(new_state);
                }
            }
        }

        // close everything and add to queue
        for (auto &[_, state_set] : nt) {
            epsilon_close(state_set, nfa);
            //std::cerr << "queuing\n";
            queue.push_front(state_set);
        }


        // update the transition map
        nfa_transitions.emplace(current_state_set, nt);
    }

    // create a nfa_state_set map to new dfa_states
    std::map<nfa_state_set, dfa_state_id> dfa_state_map;

    for (auto [state_set, _] : nfa_transitions) {
        dfa_state_map.emplace(state_set, dfa_state_id::get_next_id());
    }

    // now iterate through the transitions and
    // actually create our machine.

    std::unique_ptr<dfa_machine> retval = std::make_unique<dfa_machine>();

    for (auto [state_set, trans] : nfa_transitions) {
        auto current_dfa_id = dfa_state_map.at(state_set);

        dfa_state new_state;
        new_state.id = current_dfa_id;
        for (auto [sym, trans_state_set] : trans) {
            auto trans_to_state = dfa_state_map.at(trans_state_set);
            new_state.transitions_.emplace(sym, trans_to_state);
        }

        for (auto state_in_set : state_set) {
            auto nfa_state = nfa.states_.at(state_in_set);
            if (nfa_state.accepting) {
                new_state.accepted_symbol.insert(nfa_state.accepted_symbol);
            }
        }

        if (new_state.accepted_symbol.size() > 0) {
            new_state.accepting = true;
            retval->accepting_states_.insert(current_dfa_id);
        }

        retval->states_.emplace(current_dfa_id, new_state);

    }

    retval->start_state_ = dfa_state_map.at(start_state);

        

    return retval;
};

} // yalr::codegen
