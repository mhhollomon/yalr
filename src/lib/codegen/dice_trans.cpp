#include "codegen/dice_trans.hpp"


namespace yalr::codegen {

std::set<diced_trans_t> dice_transitions(std::set<diced_trans_t> &wood_pile) {


    std::set<diced_trans_t>finished_list;


    // walk through th list (possibly multiple times)
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
    // The set will order the transitions by length first. So, the
    // wood_pile.begin() will be one of the shortest (nothing will be
    // shorter).
    //

    while (not wood_pile.empty()) {
        auto iter = wood_pile.begin();
        auto axe = *iter;
        wood_pile.erase(iter);

        std::set<diced_trans_t> things_to_add;
        std::set<diced_trans_t> things_to_erase;

        bool made_it_to_the_end = true;
        for (auto wood_iter = wood_pile.begin();
                wood_iter != wood_pile.end();) {
            // axe will be no longer than wood.
            auto const &wood = *wood_iter;
            if (axe.low < wood.low) {
                if (axe.high < wood.low) {
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

    return finished_list;

}

}
