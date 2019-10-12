#include "tablegen.hpp"

#include <set>
#include <queue>
#include <algorithm>

/*
 * This is a fairly naive inplementation of the Simple LR parser table
 * generation algorithm given in the "Dragon Book" section 4.7.
 *
 * For the moment, no special care is taken to try to optimize the
 * algorithm - except that the goto(I,X) is partially cached in the transitions
 * member of the lrstate.
 *
 * Compilers: Principles, Techniques, and Tools
 * Aho, Sethi, Ullman
 * Copyright 1986
 */
namespace yalr {

using state_set = std::set<item_set>;

void pretty_print(const item_set& i, std::ostream& strm);
void pretty_print(const lrstate& lr, std::ostream& strm);

/* Compute the closure of the set of items
 *
 * For each item in `items`, add to the return value.
 * If the position is just before a rule in the production,
 * Add all the productions with that LHS rule to the set.
 * But these may also need to introduce other productions as well.
 */
item_set closure(
        const production_map pm,
        const item_set& items) {
    item_set retval;
    
    // use a queue to keep track of what needs to processed.
    std::queue<lr_item> q;

    // use a set to keep track of what rules we have already expanded.
    std::set<symbol_identifier_t> seen;

    // add everything from the original set to the queue.
    for (const auto& i: items) {
        q.push(i);
    }

    while (! q.empty()) {
        auto curr_item = q.front();
        q.pop();

        retval.insert(curr_item);
        auto prod = pm.at(curr_item.prod_id);
        
        // make sure we are not at the Right edge of the production.
        if (curr_item.position >= int(prod.items.size())) {
            continue;
        }

        switch (prod.items[curr_item.position].sym.type()) {
            case symbol_type::terminal :
                // its a terminal, nothing to do
                break;
            case symbol_type::skip :
                // its a skip, this isn;t right
                std::cerr << "There is a skip in a production, but analyze "
                    "was supposed to take care of that\n";
                //NOLINTNEXTLINE
                assert(false);
                break;
            case symbol_type::rule :
                {
                auto curr_rule_id = prod.items[curr_item.position].sym.id();
                if (seen.count(curr_rule_id) == 0) {
                    seen.insert(curr_rule_id);
                    for (const auto& [_, p] : pm) {
                        if (p.rule.id() == curr_rule_id) {
                            q.push(lr_item{p.prod_id, 0});
                        }
                    }
                }
                }
                break;
            default :
                std::cerr << "This isn't right -- " << curr_item.prod_id << 
                    ":"<< curr_item.position << " -- " << prod.items.size() << "\n";
                //NOLINTNEXTLINE
                assert(false);
                break;
        }
    }

    return retval;
}

/*
 * For each item in the set, assume that symbol X has just been seen. Advance
 * the position or drop the item.
 */
item_set goto_set(const production_map& pm, const item_set& I, const symbol& X) {
    item_set retval;

    for (const auto& curr_item : I) {
        auto prod = pm.at(curr_item.prod_id);
        
        // make sure we are not at the Right edge of the production.
        if (curr_item.position >= int(prod.items.size())) {
            continue;
        }

        if (prod.items[curr_item.position].sym == X) {
            retval.emplace(curr_item.prod_id, curr_item.position+1);
        }
    }

    return closure(pm, retval);
}

// Cribbed from : https://medium.com/100-days-of-algorithms/day-93-first-follow-cfe283998e3e
void compute_first_and_follow(lrtable& lt) {

    /* Initialize the sets.
     * For first :
     *      terminals get a set consisting of themselves
     *      non-terminals get an empty set.
     * For Follow :
     *      terminals do not get anything.
     *      non-terminals get an empty set.
     *          except the start symbol, which gets a set with '$' (end-of-input).
     */
    for (const auto& iter : lt.symbols) {
        if (iter.second.type() == symbol_type::terminal) {
            lt.first_set.emplace(iter.second, symbol_set{iter.second});
            // no follow set for terminals
        } else if (iter.second.type() == symbol_type::rule) {
            lt.first_set.emplace(iter.second, symbol_set{});
            auto [ m_iter, _ ] = lt.follow_set.emplace(iter.second, symbol_set{});

            if (iter.second == lt.productions[lt.target_prod].rule) {
                auto endsym = lt.symbols.find("$");
                m_iter->second.insert(*endsym);
            }
        } else {
            /* ignore skips */
        }
    }

    bool updated = true;

    // Keep running through the productions until nothing changes.
    while (updated) {
        updated = false;
        for (const auto& [_, prod] : lt.productions) {

            // can the lhs symbol produce epsilon?
            // Assume it can until we prove otherwise.
            bool is_epsilon = true;

            for (const auto& aitem : prod.items) {
                const auto &symfirst = lt.first_set[aitem.sym];

                // Add the first set of the symbol in the production to
                // the first set of the symbol this is a rule for.
                // As an optimization, only do it if there is something
                // in the set we're adding.
                if (not symfirst.empty()) {
                    updated |= lt.first_set[prod.rule].addset(symfirst);
                }

                // check to see if our current symbol can produce epsilon.
                // If it can't, we're done with this production.
                if (not lt.epsilon.contains(aitem.sym)) {
                    // if the current symbol cannot produce epsilon, then
                    // neither can the lhs symbol.
                    is_epsilon = false;
                    break;
                }
            }

            // Add the lhs to epsilon if necessary.
            // Note: since epsilon starts out empty, this is conservative.
            if (is_epsilon) {
                // bleah. add a method to symbolset to make this more graceful.
                auto [_, inserted] = lt.epsilon.insert(prod.rule);
                updated |= inserted;
            }

            /****** Follow set computation ********
             *
             * Consider a rule such as 
             * LHS => A B C
             * 
             * Starting at the end:
             * consider C.
             * If C is a non-terminal, then follow(LHS) needs to be added to
             * follow(C). 
             * If C can produce epsilson, then first(C) needs to be added to
             * follow(LHS)
             * Now consider B.
             * If B is a non-terminal, then if epsilon(C) then follow(LHS)
             * needs to added to follow(B), otherwise First(C) needs to added
             * to follow(B).
             *
             * In other words, follow(LHS) needs to propagate backwards until
             * a non-epsilon symbol is found. When that happens, the first set
             * of THAT symbol needs to propagate backwards until we hit another
             * non-epsilon symbol.
             *
             * The `aux` variable contains the set we need to propagate.
             */

            // For follow, we need to walk through each symbol in the production
            // starting at the end. So, reverse the symbols.
            // TODO - maybe use rend()/rbegin() instead. Doing the reverse each
            // time through is wasteful.
            decltype(prod.items) reversed(prod.items.size());
            std::reverse_copy(std::begin(prod.items), std::end(prod.items),
                    std::begin(reversed));

            auto *aux = &(lt.follow_set[prod.rule]);
            symbol_set tempset ;
            for (const auto& aitem : reversed) {
                if (aitem.sym.type() == symbol_type::rule) {
                    updated |= lt.follow_set[aitem.sym].addset(*aux);
                }

                if (lt.epsilon.contains(aitem.sym)) {
                    if ( aux != &tempset) {
                        tempset.clear();
                        tempset.addset(*aux);
                        tempset.addset(lt.first_set[aitem.sym]);
                        aux = &tempset;
                    } else {
                        tempset.addset(lt.first_set[aitem.sym]);
                    }
                } else {
                    // aitem.sym is non-epsilon. So start propagating
                    // its first set.
                    aux = &(lt.first_set[aitem.sym]);
                }
            }
        }
    }
}

/*
 * Main computation
 */
 std::unique_ptr<lrtable> generate_table(const analyzer_tree& g) {

    auto retval = std::make_unique<lrtable>();

    retval->symbols = g.symbols;
    retval->target_prod = g.target_prod;
    retval->options = g.options;
    
    for (auto &p : g.productions) {
        retval->productions.try_emplace(p.prod_id, p);
    }

    // new lrstates to process
    std::queue<lrstate*> q;

    //map item_set to lrstate to make sure about uniqueness
    std::map<item_set, lrstate> state_map;

    // the initial lrstate comes from the target production
    auto prod_id = g.target_prod;
    item_set I{{lr_item(prod_id, 0)}};

    item_set close = closure(retval->productions, I);

    auto [lr, placed] = state_map.emplace(std::make_pair(
                close, lrstate{close, true}));

    q.push(&(lr->second));

    while (! q.empty()) {
        auto curr_state = q.front();
        q.pop();

        /* run through all the grammar symbols (terms and rules) */
        for (const auto& [_, X] : g.symbols ) {
            if (X.isskip()) {
                continue;
            }
            auto is = goto_set(retval->productions, curr_state->items, X);

            if (is.empty()) {
                continue;
            }

            auto iter = state_map.find(is);
            if (iter != state_map.end()) {
                curr_state->transitions.emplace(std::make_pair(X, 
                            transition{X, iter->second.id}));
            } else {
                auto [iter, placed] = state_map.emplace(
                        std::make_pair(is, lrstate(is)));
                q.push(&(iter->second));
                curr_state->transitions.emplace(std::make_pair(X, 
                            transition(X, iter->second.id)));
            }

        }
    }

    /*
     * compute first and follow sets
     */

    compute_first_and_follow(*retval);

    /* Compute Actions 
     *
     */

    for (auto& iter : state_map) {
        auto &state = iter.second;
        /* Shift actions
         * Use the transitions to find those on terminals
         */
        for (const auto& t_iter : state.transitions) {
            if (t_iter.first.type() == symbol_type::rule) {
                state.gotos.emplace(t_iter.first, t_iter.second.new_state_id);

            } else if (t_iter.first.type() == symbol_type::terminal) {
                state.actions.emplace(t_iter.first, action(action_type::shift, 
                            t_iter.second.new_state_id));
            } else {
                // Other code should make sure that skips aren't
                // here. But trap it - just in case.
                //NOLINTNEXTLINE
                assert(false);
            }
        }
        /* Reduce Actions
         * Look for items with the position on the right edge.
         * Need to be on the look out for shift/reduce conflicts
         *
         * TODO - pull this out into a function to help with the nesting.
         */
        for (const auto& item : state.items) {
            const auto& prod = retval->productions[item.prod_id];
            if ( size_t(item.position) >= prod.items.size()) {
                if (item.prod_id == g.target_prod) {
                    auto  eoi = g.symbols.find("$");
                    state.actions.emplace(*eoi,
                            action(action_type::accept));
                } else {
                    /* add reduce */
                    for (const auto& sym : retval->follow_set[prod.rule]) {
                        if (sym.type() == symbol_type::terminal) {
                            auto [ new_iter, placed ] = state.actions.try_emplace(sym,
                                    action(action_type::reduce, item.prod_id));
                            if (!placed) {
                                if (new_iter->first.type() == symbol_type::terminal) {
                                    symbol shift_sym = new_iter->first;
                                    std::cerr << "Shift/reduce conflict in state " << state.id <<
                                        " between term " << shift_sym.pretty_name() << " and "
                                        << "production = " << item.prod_id << "\n";

                                    auto term_ptr = shift_sym.get_data<symbol_type::terminal>();
                                    auto term_precedence = term_ptr->precedence ? *(term_ptr->precedence) : -99;
                                    auto prod_precedence = prod.precedence ? *prod.precedence : -99;

                                    bool will_shift = (term_precedence > prod_precedence) || 
                                        ((term_precedence == prod_precedence) &&
                                             (term_ptr->associativity == assoc_type::right));
                                    bool will_reduce = (prod_precedence > term_precedence) ||
                                        ((term_precedence == prod_precedence) &&
                                             (term_ptr->associativity == assoc_type::left));

                                    if (not will_shift and not will_reduce) {
                                        std::cerr << " Forcing it to a parse time error\n";
                                        state.actions.erase(sym);
                                    } else if (will_shift) {
                                        std::cerr << " Resolving to a shift\n";
                                    } else if (will_reduce) {
                                        std::cerr << " Resolving to a reduce\n";
                                        state.actions.erase(sym);
                                        auto [ act, placed ] = state.actions.try_emplace(sym,
                                                action(action_type::reduce, item.prod_id));
                                        //NOLINTNEXTLINE
                                        assert(placed);
                                    }
                                } else {
                                    std::cerr << "Reduce/reduce conflict in state " << state.id
                                        << " between production = " << item.prod_id << " and "
                                        << "production = " << new_iter->second.production_id << "\n";

                                    auto const &old_prod = retval->productions.find(new_iter->second.production_id)->second;
                                    auto orig_precedence = old_prod.precedence ? *old_prod.precedence : -99;
                                    auto new_precedence = prod.precedence ? *prod.precedence : -99;

                                    if (new_precedence > orig_precedence) {
                                        std::cerr << " Resolving to reduce by " << item.prod_id << "\n";
                                        state.actions.erase(sym);
                                        auto [ act, placed ] = state.actions.try_emplace(sym,
                                                action(action_type::reduce, item.prod_id));
                                        //NOLINTNEXTLINE
                                        assert(placed);
                                    } else {
                                        std::cerr << " Resolving to reduce by " << old_prod.prod_id << "\n";
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }


    retval->states.reserve(state_map.size());
    for (const auto& iter : state_map) {
        retval->states.push_back(iter.second);
    }

    std::sort(retval->states.begin(), retval->states.end(), 
            [](const lrstate& a, const lrstate& b) { return (a.id < b.id); }
            );


    return retval;

}

void pretty_print(const item_set& is,
       const production_map& productions, std::ostream& strm) {
    for (const auto &i : is) {
        const auto prod = productions.at(i.prod_id);
        //NOLINTNEXTLINE
        assert(i.prod_id == prod.prod_id);

        strm << "   [" << i.prod_id << "] ";

        strm << prod.rule.name() << " => ";

        for (int index = 0; index < int(prod.items.size()); ++index) {
            if (i.position == index ) {
                strm << " *";
            }

            strm << " " << prod.items[index].sym.pretty_name();
        }
        if (i.position >= int(prod.items.size())) {
            strm << " *";
        }
        strm << "\n";
    }
}

void pretty_print(
        const lrstate& lr, 
        const production_map& productions,
        std::ostream& strm) {
    strm << "--------- State " << lr.id << " " << 
        (lr.initial ? "Initial" : "") << "\n";
    pretty_print(lr.items, productions, strm);
    
    strm << "Transitions:\n";
    for (const auto& iter : lr.transitions) {
        const auto t = iter.second;
        strm << "  " << t.on_symbol.pretty_name() << " => state " << t.new_state_id << "\n";
    }
    
    strm << "Actions:\n";
    for (const auto& iter : lr.actions) {
        strm << "  " << iter.first.pretty_name() << " => " << iter.second.type_name();
        switch(iter.second.type) {
            case action_type::shift:
               strm << " and move to state " <<  iter.second.new_state_id;
               break;
            case action_type::reduce:
               {
                   strm << " by production " << iter.second.production_id;
               }
               break;
            default :
               break;
        }
        strm << "\n";
    }

    strm << "Gotos:\n";
    for (const auto& iter : lr.gotos) {
        strm << "  " << iter.first.pretty_name() << " => state " << 
            iter.second << "\n";
    }

}

void pretty_print(const std::string& desc, const std::map<symbol, symbol_set>& s, 
        std::ostream& strm) {
    strm << desc << "\n";
    std::string pfx = "    ";

    for (const auto& m_iter : s) {
        strm << pfx << m_iter.first.pretty_name() << " :";
        for(const auto& symb : m_iter.second) {
            strm << " " << symb.name();
        }
        strm << "\n";
    }
}

void pretty_print(const lrtable& lt, std::ostream& strm) {
    pretty_print("First Set", lt.first_set, strm);
    pretty_print("Follow Set", lt.follow_set, strm);
    strm << "Epsilons:\n";
    for (const auto& symb : lt.epsilon) {
        strm << "    " << symb.name() << "\n";
    }
    for (const auto& state : lt.states) {
        pretty_print(state, lt.productions, strm);
    }
}

} // namespace yalr
