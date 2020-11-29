#include "unlap_char_ranges.hpp"

struct diced_trans_t {
    int length;
    char low;
    char high;

    diced_trans_t(char l, char h) :
        length(h-l),
        low(l), high(h) {}

    diced_trans_t(char_range const &cr) :
        length(cr.high - cr.low),
        low(cr.low),
        high(cr.high) {}

    bool operator <(diced_trans_t const &r) const {
        return ( (length < r.length) or
                (length == r.length and low < r.low ) or
                (length == r.length and low == r.low and high < r.high));
    }
};

std::set<char_range> unlap_char_ranges(std::set<char_range> const & input) {

    std::set<diced_trans_t>wood_pile;

    wood_pile.insert(input.begin(), input.end());


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
                    // so, there will be three pieces
                    // axe.low - wood.low-1
                    // wood.low - axe.high
                    // axe.high+1 - wood.high
                    wood_pile.emplace(axe.low,    wood.low-1);
                    wood_pile.emplace(wood.low,   axe.high);
                    wood_pile.emplace(axe.high+1, wood.high);

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
                    // wood.low - axe.high
                    // axe.high+1 - wood.high
                    wood_pile.emplace(wood.low,   axe.high );
                    wood_pile.emplace(axe.high+1, wood.high);

                    // delete the original wood
                    wood_iter = wood_pile.erase(wood_iter);
                }
            } else if (axe.low <= wood.high) {
                if (axe.high <= wood.high) {
                    // wood.low - axe.low-1
                    // axe.low - axe.high
                    // axe.high+1 - wood.high
                    wood_pile.emplace(wood.low, axe.low-1);
                    wood_pile.emplace(axe.low,  axe.high);
                    if (axe.high < wood.high) {
                        wood_pile.emplace(axe.high+1, wood.high);
                    }

                    // delete the original wood
                    wood_iter = wood_pile.erase(wood_iter);
                } else {
                    // shattering the axe. It straddles wood.high
                    // wood.low - axe.low-1
                    // axe.low - wood.high
                    // wood.high+1, axe.high
                    wood_pile.emplace(wood.low,    axe.low-1);
                    wood_pile.emplace(axe.low,     wood.high);
                    wood_pile.emplace(wood.high+1, axe.high);

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

    std::set<char_range> retval;

    for (auto const & x : finished_list) {
        retval.emplace(x.low, x.high);
    }

    return retval;

}
