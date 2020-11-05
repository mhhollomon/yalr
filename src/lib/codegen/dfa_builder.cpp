#include "codegen/fsm_builders.hpp"

#include <deque>

namespace yalr::codegen {

// A set of states in the nfa machine. each such set will become
// a new state in our dfa.
using nfa_state_set = std::set<nfa_state_identifier_t>;

// holds the transitions for a given state_set
using nfa_transition = std::map<input_symbol_t, nfa_state_set>;

// ------------------------------------------------------------------
// updates the given state to be the closure over epsilon
// in the given machine
void epsilon_close(nfa_state_set &states, const nfa_machine &nfa) { 

    std::deque<nfa_state_identifier_t>queue{states.begin(), states.end()};
    std::set<nfa_state_identifier_t>seen;

    while (not queue.empty()) {
        auto current_id = queue.back();
        queue.pop_back();
        if (seen.count(current_id) > 0)  {
            continue;
        }
        seen.insert(current_id);
        auto &current_state = nfa.states_.at(current_id);
        auto [lower, upper] = current_state.transitions_.equal_range(epsilon_symbol);
        for(auto iter = lower; iter != upper; ++iter) {
            states.insert(iter->second);
            if (seen.count(iter->second) == 0) {
                queue.push_front(iter->second);
            }
        }
    }
}

// ------------------------------------------------------------------
struct diced_trans_t {
    nfa_state_identifier_t to_state_id;
    int length;
    symbol_type_t sym_type;
    char low;
    char high;

    diced_trans_t(char l, char h, nfa_state_identifier_t id) :
        to_state_id(id), length(h-l),
        sym_type(sym_char), 
        low(l), high(h) {}

    bool operator <(diced_trans_t const &r) const {
        return ( (length < r.length) or
                (length == r.length and low < r.low ) or
                (length == r.length and low == r.low and high < r.high) or
                (length == r.length and low == r.low and high == r.high and to_state_id < r.to_state_id));
    }
};

auto compute_diced_transitions(nfa_state_set const &states, const nfa_machine &nfa) {
    // using a set because of the ordering guarantees.
    // work_list.begin() will be no longer than any other.
    //
    std::set<diced_trans_t> wood_pile;

    std::set<diced_trans_t>finished_list;

    // pull all the transitions available to this set of states into 
    // our set.
    for (auto state_id : states) {
        auto const & state = nfa.states_.at(state_id);

        for(auto const &[sym, to_id] : state.transitions_) {
            if (sym.sym_type_ == sym_epsilon) { continue; }
            wood_pile.emplace(sym.first_, sym.last_, to_id);
        }
    }

    // Now, walk through th list (possibly multiple times)
    // split overlapping ranges so that if e.g.
    //      b --> 1
    //      a..d --> 2
    //  becomes
    //      b --> 1
    //      a --> 2
    //      b --> 2
    //      c..d --> 2
    //
    // yes this is O(n^^2)
    //

    while (not wood_pile.empty()) {
        auto iter = wood_pile.begin();
        auto axe = *iter;
        wood_pile.erase(iter);

        std::set<diced_trans_t> things_to_add;
        std::set<diced_trans_t> things_to_erase;

        bool made_it_to_the_end = true;
        for (auto wood_iter = wood_pile.begin(); wood_iter != wood_pile.end();) {
            // axe will be no longer than wood.
            auto const &wood = *wood_iter;
            if (axe.low < wood.low) {
                if (axe.high < wood.high) {
                    // no overlap
                    ++wood_iter;
                } else {
                    // axe.high must be less than wood.high
                    // so, there will be up to four pieces. 
                    // axe.low - wood.low-1 - with axe.state
                    // wood.low - axe.high - with axe.state
                    // wood.low - axe.high - with wood.state
                    // axe.high+1 - wood.high - with wood.state
                    wood_pile.emplace(axe.low, wood.low-1, axe.to_state_id);
                    wood_pile.emplace(wood.low, axe.high, axe.to_state_id);
                    if (wood.to_state_id != axe.to_state_id) {
                        wood_pile.emplace(wood.low, axe.high, wood.to_state_id);
                    }
                    wood_pile.emplace(axe.high+1, wood.high, wood.to_state_id);

                    // delete the original wood (axe is already out)
                    wood_iter = wood_pile.erase(wood_iter);

                    // we shattered the axe so need to go back to the top
                    made_it_to_the_end = false;
                    break; // out of the wood loop
                }
            } else if (axe.low == wood.low) {
                if (axe.high == wood.high) {
                    // the ranges are the same, but the states must be different.
                    // nothing to do.
                    ++wood_iter;
                } else {
                    // axe is contained totally in wood
                    // wood.low - axe.high - with wood.state
                    // axe.high+1 - wood.high - with wood.state
                    if (wood.to_state_id != axe.to_state_id) {
                        wood_pile.emplace(wood.low, axe.high, wood.to_state_id);
                    }
                    wood_pile.emplace(axe.high+1, wood.high, wood.to_state_id);
                    
                    // delete the original wood
                    wood_iter = wood_pile.erase(wood_iter);
                }
            } else if (axe.low <= wood.high) {
                if (axe.high <= wood.high) {
                    // wood.low - axe.low-1 - wood.state
                    // axe.low - axe.high - wood.state
                    // axe.high+1 - wood.high - wood.state (may be empty)
                    wood_pile.emplace(wood.low, axe.low-1, wood.to_state_id);
                    if (wood.to_state_id != axe.to_state_id) {
                        wood_pile.emplace(axe.low, axe.high, wood.to_state_id);
                    }
                    if (axe.high < wood.high) {
                        wood_pile.emplace(axe.high+1, wood.high,  wood.to_state_id);
                    }
                    
                    // delete the original wood
                    wood_iter = wood_pile.erase(wood_iter);
                } else {
                    // shattering the axe.
                    // wood.low - axe.low-1 - wood.state
                    // axe.low - wood.high - axe.state
                    // axe.low - wood.high - wood.state
                    // wood.high+1, axe.high - axe.state
                    wood_pile.emplace(wood.low, axe.low-1, wood.to_state_id);
                    wood_pile.emplace(axe.low, wood.high, axe.to_state_id);
                    if (wood.to_state_id != axe.to_state_id) {
                        wood_pile.emplace(axe.low, wood.high,  wood.to_state_id);
                    }
                    wood_pile.emplace(wood.high+1, axe.high, axe.to_state_id);
                    
                    // delete the original wood
                    wood_iter = wood_pile.erase(wood_iter);
                    
                    // we shattered the axe so need to go back to the top
                    made_it_to_the_end = false;
                    break; // out of the wood loop
                }
            } else {
                // no overlap. nothing to do
                ++wood_iter;
            }

        }

        if (made_it_to_the_end) {
            finished_list.insert(axe);
        }
    }

    auto retval = std::make_unique<std::multimap<input_symbol_t, nfa_state_identifier_t>>();
    for (auto const &t : finished_list) {
        //std::cout << "diced : " << t.low << " - " << t.high << " --> " << t.to_state_id << "\n";
        retval->emplace(input_symbol_t{t.sym_type, t.low, t.high}, t.to_state_id);
    }

    return retval;

}



// -------------------------------------------------------------
// Use standard power set generation to turn the states of the
// nfa to the states for our DFA.
//
std::unique_ptr<dfa_machine> build_dfa(const nfa_machine &nfa) {

    // This is our DFA in terms of the nfa states. For each
    // state set, what are the transitions.
    //
    std::map<nfa_state_set, nfa_transition> nfa_transitions;

    // create the new start state
    nfa_state_set start_state;
    start_state.insert(nfa.start_state_id_);
    epsilon_close(start_state, nfa);


    // find sets until we stop finding new ones
    //
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

        // we've already seen this set
        if (nfa_transitions.count(current_state_set) > 0 ) continue;

        // build transitions for this state set
        nfa_transition nt;

        auto transition_list = compute_diced_transitions(current_state_set, nfa);

        for (auto const &[sym, to_state_id] : *transition_list) {
            auto iter = nt.find(sym);
            if (iter == nt.end() ) {
                auto new_set = nfa_state_set{};
                new_set.insert(to_state_id);
                nt.emplace(sym, new_set);
            } else {
                iter->second.insert(to_state_id);
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
    std::map<nfa_state_set, dfa_state_identifier_t> dfa_state_map;

    for (auto [state_set, _] : nfa_transitions) {
        dfa_state_map.emplace(state_set, dfa_state_identifier_t::get_next_id());
    }

    // ----------------------------------------------------------------
    // now iterate through the transitions and
    // actually create our machine.
    //

    std::unique_ptr<dfa_machine> retval = std::make_unique<dfa_machine>();

    for (auto const &[state_set, trans] : nfa_transitions) {
        auto current_dfa_id = dfa_state_map.at(state_set);

        dfa_state new_state;
        new_state.id_ = current_dfa_id;
        for (auto [sym, trans_state_set] : trans) {
            auto trans_to_state = dfa_state_map.at(trans_state_set);
            new_state.transitions_.emplace(sym, trans_to_state);
            //auto [iter, _] = new_state.transitions_.emplace(sym, trans_to_state);
            //std::cout << "dfa : " << current_dfa_id << " { " << iter->first.first_ << ", " << iter->first.last_ << "} --> " << iter->second << "\n";
        }

        for (auto state_in_set : state_set) {
            auto nfa_state = nfa.states_.at(state_in_set);
            if (nfa_state.accepting_) {
                new_state.accepted_symbol_.insert(nfa_state.accepted_symbol_);
            }
        }

        if (new_state.accepted_symbol_.size() > 0) {
            new_state.accepting_ = true;
            retval->accepting_states_.insert(current_dfa_id);
        }

        retval->states_.emplace(current_dfa_id, new_state);

    }

    retval->start_state_ = dfa_state_map.at(start_state);

        

    return retval;
};

} // yalr::codegen
