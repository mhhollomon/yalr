#include "algo/slr_parse_table.hpp"
#include "algo/slr.hpp"

#include "yassert.hpp"

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
namespace yalr::algo {

using state_set = std::set<item_set>;

// Lowest precedence
// The user can't give us a negative precedence
constexpr int low_prec = -99;

//
constexpr std::streamsize min_max_name_len = 5;
constexpr std::streamsize max_max_name_len = 15;

//
constexpr int prec_print_width = 5;
constexpr int assoc_print_width = 5;
constexpr int sym_id_width = 3;

/* Compute the closure of the set of items
 *
 * For each item in `items`, add to the return value.
 * If the position is just before a rule in the production,
 * Add all the productions with that LHS rule to the set.
 * But these may also need to introduce other productions as well.
 */
item_set closure(
        const production_map &pm,
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
                yfail("There is a skip in a production during table generation");
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
                yfail("Unkown symbol type in production");
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
void compute_first_and_follow(slr_parse_table& lt) {

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
 std::unique_ptr<gen_results> 
 slr_generator::generate_table(const analyzer_tree& t) {

    int error_count = 0;
    this->lrtable = std::make_unique<slr_parse_table>();

    lrtable->symbols = t.symbols;
    lrtable->target_prod = t.target_prod;
    lrtable->options = t.options;
    lrtable->verbatim_map = t.verbatim_map;
    lrtable->version_string = t.version_string;
    
    for (const auto &p : t.productions) {
        lrtable->productions.try_emplace(p.prod_id, p);
    }

    // new lrstates to process
    std::queue<lrstate*> q;

    //map item_set to lrstate to make sure about uniqueness
    std::map<item_set, lrstate> state_map;

    // the initial lrstate comes from the target production
    auto prod_id = t.target_prod;
    item_set I{{lr_item(prod_id, 0)}};

    item_set close = closure(lrtable->productions, I);

    auto [lr, placed] = state_map.emplace(std::make_pair(
                close, lrstate{close, true}));

    q.push(&(lr->second));

    while (! q.empty()) {
        auto *curr_state = q.front();
        q.pop();

        /* run through all the grammar symbols (terms and rules) */
        for (const auto& [_, X] : t.symbols ) {
            if (X.isskip()) {
                continue;
            }
            auto is = goto_set(lrtable->productions, curr_state->items, X);

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

    compute_first_and_follow(*lrtable);

    /* Compute Actions 
     *
     */

    for (auto& iter : state_map) {
        auto &state = iter.second;
        /* Shift actions
         * Use the transitions to find those on terminals
         *
         * By doing the shifts first, we make sure that all
         * shift/reduce conflicts happen when trying to add the reduce.
         * This eases the task of reproting and applying precedence.
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
                yfail("Invalid symbol type in transition");
            }
        }
        /* Reduce Actions
         * Look for items with the position on the right edge.
         * Need to be on the look out for shift/reduce conflicts
         *
         * TODO - pull this out into a function to help with the nesting.
         */
        for (const auto& item : state.items) {
            const auto& prod = lrtable->productions[item.prod_id];
            if ( size_t(item.position) >= prod.items.size()) {
                if (item.prod_id == t.target_prod) {
                    auto  eoi = t.symbols.find("$");
                    state.actions.emplace(*eoi,
                            action(action_type::accept));
                } else {
                    /* add reduce */
                    for (const auto& sym : lrtable->follow_set[prod.rule]) {
                        if (sym.type() == symbol_type::terminal) {
                            auto [ new_iter, placed ] = state.actions.try_emplace(sym,
                                    action(action_type::reduce, item.prod_id));
                            if (!placed) {

                                symbol trans_symbol = new_iter->first;
                                auto &old_action = new_iter->second;
                                auto *term_ptr = trans_symbol.get_data<symbol_type::terminal>();

                                if (old_action.type == action_type::shift) {
                                    //
                                    // Shift/Reduce Conflict
                                    //

                                    auto term_precedence = term_ptr->precedence ? *(term_ptr->precedence) : low_prec;
                                    auto prod_precedence = prod.precedence ? *prod.precedence : low_prec;
                                    std::cerr << "Shift/Reduce conflict between shift " <<  trans_symbol.name() << " and production " << prod.prod_id << "\n";

                                    bool will_shift = (term_precedence > prod_precedence) || 
                                        ((term_precedence == prod_precedence) &&
                                             (term_ptr->associativity == assoc_type::right));
                                    bool will_reduce = (prod_precedence > term_precedence) ||
                                        ((term_precedence == prod_precedence) &&
                                             (term_ptr->associativity == assoc_type::left));

                                    if (not will_shift and not will_reduce) {
                                        std::cerr << "Shift/reduce conflict in state " << state.id <<
                                            " between term " << trans_symbol.name() << " and "
                                            << "production = " << item.prod_id << "\n";
                                        error_count += 1;

                                        old_action.conflict = conflict_action(action_type::reduce,
                                                item.prod_id);
                                        old_action.conflict->resolved = false;

                                    } else if (will_shift) {
                                        old_action.conflict = conflict_action(action_type::reduce,
                                                item.prod_id);

                                    } else if (will_reduce) {
                                        std::cerr << "Deciding to reduce\n";
                                        action new_act{action_type::reduce, item.prod_id};
                                        // TODO - clean this up by putting some of the constructors
                                        // in a separate cpp file along with the pretty printing stuff.
                                        new_act.conflict = conflict_action(old_action.type, 
                                                old_action.new_state_id);
                                        state.actions.erase(sym);
                                        auto [ act, placed ] = state.actions.try_emplace(sym,
                                                new_act);
                                        yassert(placed, "Could not place new action on shift/reduce conflict resolution");
                                    }
                                } else {
                                    //
                                    // Reduce/Reduce conflict
                                    //
                                    auto const &old_prod = lrtable->productions.find(new_iter->second.production_id)->second;
                                    std::cerr << "Reduce/Reduce conflict between " << old_prod.prod_id << " and " << item.prod_id << "\n";
                                    auto orig_precedence = old_prod.precedence ? *old_prod.precedence : low_prec;
                                    auto new_precedence = prod.precedence ? *prod.precedence : low_prec;

                                    std::cerr << "orig prec = " << orig_precedence << "\n";
                                    std::cerr << "new  prec = " << new_precedence << "\n";

                                    if (new_precedence > orig_precedence) {
                                        std::cerr << "choosing new\n";
                                        action new_act{action_type::reduce, item.prod_id};
                                        new_act.conflict = conflict_action(action_base(old_action));
                                        state.actions.erase(sym);
                                        auto [ act, placed ] = state.actions.try_emplace(sym,new_act);
                                        yassert(placed, "Could not place new action on reduce/reduce conflict resolution");
                                    } else {
                                        std::cerr << "choosing old\n";
                                        old_action.conflict = conflict_action{action_type::reduce, prod.prod_id};

                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }


    lrtable->states.reserve(state_map.size());
    for (const auto& iter : state_map) {
        lrtable->states.push_back(iter.second);
    }

    std::sort(lrtable->states.begin(), lrtable->states.end(), 
            [](const lrstate& a, const lrstate& b) { return (a.id < b.id); }
            );

    auto retval = std::make_unique<gen_results>();

    retval->errors = std::move(this->lrtable->errors);
    retval->success = (error_count == 0);


    return retval;

}

/*******************************************
 *
 *  Pretty Printing Routines
 *
 *******************************************/
void pretty_print(const item_set& is,
       const production_map& productions, std::ostream& strm) {
    for (const auto &i : is) {
        const auto &prod = productions.at(i.prod_id);
        //NOLINTNEXTLINE
        yassert(i.prod_id == prod.prod_id, "Did not find correct production");

        strm << "   [";
        auto oldwidth = strm.width(3);
        strm << i.prod_id << "] ";
        strm.width(oldwidth);

        strm << prod.rule.name() << " => ";

        for (int index = 0; index < int(prod.items.size()); ++index) {
            if (i.position == index ) {
                strm << " |*|";
            }

            strm << " " << prod.items[index].sym.name();
        }
        if (i.position >= int(prod.items.size())) {
            strm << " |*|";
        }
        strm << "\n";
    }
}

void pretty_print( const production_map& productions, std::ostream& strm) {
    std::streamsize max_name_len = min_max_name_len;
    for (const auto &[_, p] : productions) {
        max_name_len = std::max(max_name_len, std::streamsize(p.rule.name().size()));
    }

    if (max_name_len > max_max_name_len) { 
        max_name_len = max_max_name_len;
    }

    for (const auto &[_, p] : productions) {
        strm << "   [";
        strm.width(3);
        strm << p.prod_id << "] ";

        strm.width(max_name_len);

        strm << p.rule.name() << " => ";
        if (p.precedence) {
            strm << "(" << *p.precedence << ")";
        }
        for (auto const &i : p.items) {
            strm << ' ' << i.sym.name();
        }
        strm << "\n";
    }
}

void pretty_print( const action& a, const std::string& prefix, std::ostream& strm) {
    strm << " => " << a.type_name();
    switch(a.type) {
        case action_type::shift:
            strm << " and move to state " <<  a.new_state_id;
            break;
        case action_type::reduce:
            {
                strm << " by production " << a.production_id;
            }
            break;
        default :
            break;
    }
    strm << "\n";

    if (a.conflict) {
        strm << prefix << "Conflict" << (a.conflict->resolved?"(resolved)":"") <<
            ": " << a.conflict->type_name();
        switch(a.conflict->type) {
            case action_type::shift:
                strm << " and move to state " <<  a.conflict->new_state_id;
                break;
            case action_type::reduce:
                {
                    strm << " by production " << a.conflict->production_id;
                }
                break;
            default :
                break;
        }
        strm << "\n";
    }
}
void pretty_print(
        const lrstate& lr, const production_map& productions,
        std::ostream& strm) {
    strm << "--------- State " << lr.id << " " << 
        (lr.initial ? "Initial" : "") << "\n\n";
    strm << "Items:\n";
    pretty_print(lr.items, productions, strm);
    
    /*
    strm << "Transitions:\n";
    for (const auto& iter : lr.transitions) {
        const auto t = iter.second;
        strm << "  " << t.on_symbol.name() << " => state " << t.new_state_id << "\n";
    }
    */
    
    strm << "\nActions:\n";
    for (const auto& iter : lr.actions) {
        strm << "  " << iter.first.name() ;
        pretty_print(iter.second, "     ", strm);
    }

    strm << "\nGotos:\n";
    for (const auto& iter : lr.gotos) {
        strm << "  " << iter.first.name() << " => state " << 
            iter.second << "\n";
    }

}

void pretty_print(const std::string& desc, const std::map<symbol, symbol_set>& s, 
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

void pretty_print(const symbol &sym, std::ostream& strm, std::streamsize name_width=0) {
    strm << "[";
    strm.width(sym_id_width);
    strm << sym.id() << "] ";
    if (name_width != 0) {
        strm.width(name_width);
    }
    strm << std::left << sym.name() << std::right;
    switch(sym.type()) {
        case symbol_type::terminal :
            {
                auto *data = sym.get_data<symbol_type::terminal>();
                if (data->precedence) {
                    strm << " p=";
                    strm.width(prec_print_width);

                    strm << std::left;
                    strm << *(data->precedence);
                    strm << std::right;
                } else {
                    strm << "        ";
                }


                std::string assoc_str[] = { "undef", "left", "right" };

                if (data->associativity == assoc_type::undef) {
                    strm << "        ";
                } else {
                    strm << " a=";
                    strm.width(assoc_print_width);
                    strm << std::left;
                    strm << assoc_str[int(data->associativity)];
                    strm << std::right;
                }

                if (data->type_str != "void") {
                    strm << ' ' << data->type_str;
                }
            }
            break;
        case symbol_type::rule :
            {
                auto *data = sym.get_data<symbol_type::rule>();
                if (data->type_str != "void") {
                    strm << "                 " << data->type_str;
                }

            }
            break;
        default :
            yfail("Invalid symbol type");

    }
    strm << "\n";
}

void slr_generator::output_parse_table(std::ostream& strm) {
    const slr_parse_table & lt = *lrtable;
    /*
    pretty_print("First Set", lt.first_set, strm);
    strm << "\n";
    pretty_print("Follow Set", lt.follow_set, strm);
    strm << "\n";
    strm << "Epsilons:\n";
    for (const auto& symb : lt.epsilon) {
        strm << "    " << symb.name() << "\n";
    }
    strm << "\n";
    */
    strm << "============= TOKENS ========================\n\n";
    std::streamsize max_name_len = min_max_name_len;
    for (auto const &[id, sym] : lt.symbols) {
        if (sym.isterm() or sym.isrule()) {
            max_name_len = std::max(max_name_len, std::streamsize(sym.name().size()));
        }
    }

    if (max_name_len > max_max_name_len) {
        max_name_len = max_max_name_len;
    }

    for (auto const &[id, sym] : lt.symbols) {
        if (sym.isterm() or sym.isrule()) {
            strm << "   ";
            pretty_print(sym, strm, max_name_len);
        }
    }
    strm << "\n";
    strm << "============= PRODUCTIONS ===================\n\n";
    pretty_print(lt.productions, strm);
    strm << "\n";
    strm << "============= STATES ========================\n\n";
    for (const auto& state : lt.states) {
        pretty_print(state, lt.productions, strm);
        strm << "\n";
    }
}

} // namespace yalr::algo
