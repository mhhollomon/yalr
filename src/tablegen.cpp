#include "tablegen.hpp"

#include <set>
#include <queue>
#include <algorithm>

/*
 * This is a fairly naive inplementation of the Simple LR parser table
 * generation algorithm given the the "Dragon Book" section 4.7.
 *
 * For the moment, no special care is taken to try to optimize the
 * algorithm - except that the goto(I,X) is partially cached in the transitions
 * member of the lrstate.
 *
 * Compilers: Principles, Techniques, and Tools
 * Aho, Sethi, Ullman
 * Copyright 1986
 */
namespace yalr::tablegen {

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
item_set closure(const analyzer::grammar& g, const item_set& items) {
    item_set retval;
    
    // use a queue to keep track of what needs to processed.
    std::queue<item> q;

    // use a set to keep track of what rules we hae already expanded.
    std::set<int> seen;

    // add everything from the original set to the queue.
    for (const auto& i: items) {
        q.push(i);
    }

    while (! q.empty()) {
        auto curr_item = q.front();
        q.pop();

        retval.insert(curr_item);
        auto prod = g.productions[curr_item.prod_id];
        
        // make sure we are not at the Right edge of the production.
        if (curr_item.position >= int(prod.syms.size())) {
            continue;
        }

        switch (prod.syms[curr_item.position].stype()) {
            case SymbolTable::symbol_type::term :
                // its a terminal, nothing to do
                break;
            case SymbolTable::symbol_type::skip :
                // its a skip, this isn;t right
                std::cerr << "There is a skip in a production, but analyze "
                    "was supposed to take care of that\n";
                //NOLINTNEXTLINE
                assert(false);
                break;
            case SymbolTable::symbol_type::rule :
                {
                auto curr_rule_id = prod.syms[curr_item.position].id();
                if (seen.count(curr_rule_id) == 0) {
                    seen.insert(curr_rule_id);
                    for (const auto& p: g.productions) {
                        if (p.rule.id() == curr_rule_id) {
                            q.push(item{p.prod_id, 0});
                        }
                    }
                }
                }
                break;
            default :
                std::cerr << "This isn't right -- " << curr_item.prod_id << 
                    ":"<< curr_item.position << " -- " << prod.syms.size() << "\n";
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
item_set goto_set(const analyzer::grammar& g, const item_set& I, const symbol& X) {
    item_set retval;

    for (const auto& curr_item : I) {
        auto prod = g.productions[curr_item.prod_id];
        
        // make sure we are not at the Right edge of the production.
        if (curr_item.position >= int(prod.syms.size())) {
            continue;
        }

        if (prod.syms[curr_item.position] == X) {
            retval.emplace(curr_item.prod_id, curr_item.position+1);
        }
    }

    return closure(g, retval);
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
    for (const auto& iter : lt.syms) {
        if (iter.second.stype() == SymbolTable::symbol_type::term) {
            lt.firstset.emplace(iter.second, symbolset{iter.second});
            // no follow set for terminals
        } else if (iter.second.stype() == SymbolTable::symbol_type::rule) {
            lt.firstset.emplace(iter.second, symbolset{});
            auto [ m_iter, _ ] = lt.followset.emplace(iter.second, symbolset{});

            if (iter.second == lt.productions[lt.target_prod_id].rule) {
                auto [ _, endsym ] = lt.syms.find("$");
                m_iter->second.insert(endsym);
            }
        } else {
            /* ignore skips */
        }
    }

    bool updated = true;

    // Keep running through the productions until nothing changes.
    while (updated) {
        updated = false;
        for (const auto& prod : lt.productions) {

            // can the lhs symbol produce epsilon?
            // Assume it can until we prove otherwise.
            bool is_epsilon = true;

            for (const auto& symb : prod.syms) {
                const auto &symfirst = lt.firstset[symb];

                // Add the first set of the symbol in the production to
                // the first set of the symbol this is a rule for.
                // As an optimization, only do it if there is something
                // in the set we're adding.
                if (not symfirst.empty()) {
                    updated |= lt.firstset[prod.rule].addset(symfirst);
                }

                // check to see if our current symbol can produce epsilon.
                // If it can't, we're done with this production.
                if (not lt.epsilon.contains(symb)) {
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
            decltype(prod.syms) reversed(prod.syms.size());
            std::reverse_copy(std::begin(prod.syms), std::end(prod.syms),
                    std::begin(reversed));

            auto *aux = &(lt.followset[prod.rule]);
            symbolset tempset ;
            for (const auto& symb : reversed) {
                if (symb.stype() == SymbolTable::symbol_type::rule) {
                    updated |= lt.followset[symb].addset(*aux);
                }

                if (lt.epsilon.contains(symb)) {
                    if ( aux != &tempset) {
                        tempset.clear();
                        tempset.addset(*aux);
                        tempset.addset(lt.firstset[symb]);
                        aux = &tempset;
                    } else {
                        tempset.addset(lt.firstset[symb]);
                    }
                } else {
                    // symb is non-epsilon. So start propagating
                    // its first set.
                    aux = &(lt.firstset[symb]);
                }
            }
        }
    }
}

/*
 * Main computation
 */
 std::unique_ptr<lrtable> generate_table(const analyzer::grammar& g) {
    // generate unique ids for lrstates
    int state_id = -1;

    // new lrstates to process
    std::queue<lrstate*> q;

    //map item_set to lrstate to make sure about uniqueness
    std::map<item_set, lrstate> state_map;

    // the initial lrstate comes from the target production
    auto prod_id = g.target_prod;
    item_set I = {{prod_id, 0}};

    item_set close = closure(g, I);

    auto [lr, placed] = state_map.emplace(std::make_pair(
                close, lrstate(++state_id, close, true)));

    q.push(&(lr->second));

    while (! q.empty()) {
        auto curr_state = q.front();
        q.pop();

        /* run through all the grammar symbols (terms and rules) */
        for (const auto& [_, X] : g.syms ) {
            if (X.isskip()) {
                continue;
            }
            auto is = goto_set(g, curr_state->items, X);

            if (is.empty()) {
                continue;
            }

            auto iter = state_map.find(is);
            if (iter != state_map.end()) {
                curr_state->transitions.emplace(std::make_pair(X, 
                            transition(X, iter->second.id)));
            } else {
                auto [iter, placed] = state_map.emplace(
                        std::make_pair(is, lrstate(++state_id, is)));
                q.push(&(iter->second));
                curr_state->transitions.emplace(std::make_pair(X, 
                            transition(X, iter->second.id)));
            }

        }
    }

    auto retval = std::make_unique<lrtable>();

    retval->syms = g.syms;
    retval->productions = g.productions;
    retval->target_prod_id = g.target_prod;
    retval->parser_class = g.parser_class;
    
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
            if (t_iter.first.stype() == SymbolTable::symbol_type::rule) {
                state.gotos.emplace(t_iter.first, t_iter.second.state_id);

            } else if (t_iter.first.stype() == SymbolTable::symbol_type::term) {
                state.actions.emplace(t_iter.first, action(action_type::shift, 
                            t_iter.second.state_id, 0));
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
            const auto& prod = g.productions[item.prod_id];
            if ( item.position >= prod.syms.size()) {
                if (item.prod_id == g.target_prod) {
                    auto [ _, eoi ] = g.syms.find("$");
                    state.actions.emplace(eoi,
                            action(action_type::accept, 0, 0));
                } else {
                    /* add reduce */
                    for (const auto& sym : retval->followset[prod.rule]) {
                        if (sym.stype() == SymbolTable::symbol_type::term) {
                            auto [ new_iter, placed ] = state.actions.try_emplace(sym,
                                    action(action_type::reduce, 0, item.prod_id));
                            if (!placed) {
                                if (new_iter->first.stype() == SymbolTable::symbol_type::term) {
                                    std::cerr << "Shift/reduce conflict in state ";
                                } else {
                                    std::cerr << "Reduce/reduce conflict in state ";
                                }
                                std::cerr << state.id << " for symbol " << sym.name() << "\n";
                                std::cerr << "Forcing the reduce for now\n";
                                state.actions.erase(sym);
                                auto [ act, placed ] = state.actions.try_emplace(sym,
                                        action(action_type::reduce, 0, item.prod_id));
                                //NOLINTNEXTLINE
                                assert(placed);
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
       const std::vector<analyzer::production>& productions, std::ostream& strm) {
    for (const auto &i : is) {
        const auto prod = productions[i.prod_id];
        //NOLINTNEXTLINE
        assert(i.prod_id == prod.prod_id);

        strm << "   [" << i.prod_id << "] ";

        strm << prod.rule.name() << " => ";

        for (int index = 0; index < int(prod.syms.size()); ++index) {
            if (i.position == index ) {
                strm << " *";
            }
            strm << " " << prod.syms[index].name();
        }
        if (i.position >= int(prod.syms.size())) {
            strm << " *";
        }
        strm << "\n";
    }
}

void pretty_print(
        const lrstate& lr, 
        const std::vector<analyzer::production>& productions,
        std::ostream& strm) {
    strm << "--------- State " << lr.id << " " << 
        (lr.initial ? "Initial" : "") << "\n";
    pretty_print(lr.items, productions, strm);
    
    strm << "Transitions:\n";
    for (const auto& iter : lr.transitions) {
        const auto t = iter.second;
        strm << "  " << t.sym.name() << " => state " << t.state_id << "\n";
    }
    
    strm << "Actions:\n";
    for (const auto& iter : lr.actions) {
        strm << "  " << iter.first.name() << " => " << iter.second.type_name();
        switch(iter.second.atype) {
            case action_type::shift:
               strm << " and move to state " <<  iter.second.new_state_id;
               break;
            case action_type::reduce:
               {
                   strm << " by production " << iter.second.prod_id;
               }
               break;
            default :
               break;
        }
        strm << "\n";
    }

    strm << "Gotos:\n";
    for (const auto& iter : lr.gotos) {
        strm << "  " << iter.first.name() << " => state " << 
            iter.second << "\n";
    }

}

void pretty_print(const std::string& desc, const std::map<symbol, symbolset>& s, 
        std::ostream& strm) {
    strm << desc << "\n";
    std::string pfx = "    ";

    for (const auto& m_iter : s) {
        strm << pfx << m_iter.first.name() << " :";
        for(const auto& symb : m_iter.second) {
            strm << " " << symb.name();
        }
        strm << "\n";
    }
}

void pretty_print(const lrtable& lt, std::ostream& strm) {
    strm << "parser_class = '" << lt.parser_class << "'\n";
    pretty_print("First Set", lt.firstset, strm);
    pretty_print("Follow Set", lt.followset, strm);
    strm << "Epsilons:\n";
    for (const auto& symb : lt.epsilon) {
        strm << "    " << symb.name() << "\n";
    }
    for (const auto& state : lt.states) {
        pretty_print(state, lt.productions, strm);
    }
}

} // namespace yalr::tablegen
