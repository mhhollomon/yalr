#include "analyzer.hpp"

#include "option_defaults.hpp"

namespace yalr::analyzer {

//
// Phase I visitor
//
// This pass is basically just to build the symbol table. Once we've got all
// the symbols we can do other things.
//
struct phase_i_visitor {
    yalr::analyzer_tree &out;
    int error_count = 0;
    int rule_count = 0;

    std::optional<rule> goal_rule = std::nullopt;

    explicit phase_i_visitor(yalr::analyzer_tree& g_) : out(g_) {};

    //
    // Handle rules.
    // Add the rule name to the symbol table, keep track of the
    // goal rule. Make sure that no rule has been defined twice.
    // (may need to loosen this to allow the addition of productions
    // to a rule)
    //
    void operator()(const yalr::rule &r) {
        auto [ inserted, new_sym ] = out.symbols.add(r.name.text, rule_symbol{r});

        if (! inserted ) {
            std::cerr << "'" << r.name <<
                "' has already been defined as a " <<
                new_sym.type_name() << "\n";
            error_count += 1;
        }

        if (r.isgoal) {
            if (goal_rule) {
                std::cerr << "'" << r.name.text <<
                    "' is marked as a goal rule, but '" <<
                    goal_rule->name.text << "' is already marked\n";
                error_count += 1;
            } else {
                goal_rule = r;
            }
        }
    }

    //
    // Handle Terminals.
    // Add the name and the pattern(if string and not regex) to
    // the symbol table. Check that an action has been given if the
    // term has a non-void type. Check for dups.
    //
    void operator()(const yalr::terminal &t) {

        terminal_symbol ts{t};

        if (t.type_str) {
            if (t.type_str->text != "void" and not t.action) {
                std::cerr << "'" << t.name.text <<
                    "' has non-void type but no action to assign a "
                    " value\n";
                error_count += 1;
            }
        } else {
            ts.type_str = "void"sv;
        }

        // insert the name
        auto [ inserted, new_sym ] = out.symbols.add(ts.name, ts);

        if (! inserted ) {
            std::cerr << "'" << t.name <<
                "' has already been defined as a " <<
                new_sym.type_name() << "\n";
            error_count += 1;
        }

        // insert the pattern (maybe)
        if (ts.pattern[0] == '\'') {
            auto [ inserted, pattsym ] = out.symbols.register_key(ts.pattern, new_sym);

            if (! inserted ) {
                std::cerr << "pattern '" << ts.pattern <<
                    "' has already been defined by " <<
                    pattsym.type_name() <<" " << pattsym.name() << "\n";
                error_count += 1;
            }
        }



    }

    //
    // Handle skips.
    // Add the name and (maybe) pattern to the symbol table.
    // Check for name clashes.
    //
    void operator()(const yalr::skip &t) {
        skip_symbol ss(t);
        auto [ inserted, new_sym ] = out.symbols.add(ss.name, ss);

        if (! inserted ) {
            std::cerr << "'" << ss.name <<
                "' has already been defined as a " <<
                new_sym.type_name() << "\n";
            error_count += 1;
        }
        // insert the pattern (maybe)
        if (ss.pattern[0] == '\'') {
            auto [ inserted, pattsym ] = out.symbols.register_key(ss.pattern, new_sym);

            if (! inserted ) {
                std::cerr << "pattern '" << ss.pattern <<
                    "' has already been defined by " <<
                    pattsym.type_name() <<" " << pattsym.name() << "\n";
                error_count += 1;
            }
        }
    }

    //
    // Handle options.
    //
    void operator()(const yalr::option &t) {
    }

};

//
// Phase II visitor
//
// Focused on the rules. Link the items from the rule_defs to actual symobls.
//
struct prod_visitor {
    yalr::analyzer_tree &out;
    int error_count = 0;
    int inline_terminal_count = 0;

    explicit prod_visitor(yalr::analyzer_tree &g_) : out(g_) {};

    void operator()(const yalr::rule &r) {
        /* run through each alternative and generate a production.
         * Make sure every referenced item exists.
         */
        auto rsym = out.symbols.find(r.name.text);
        //NOLINTNEXTLINE
        assert(rsym); // if not found, something went wrong up above.

        for (const auto& alt : r.alternatives) {
            std::vector<yalr::prod_item> s;
            for (const auto& p : alt.items) {
                auto sym = out.symbols.find(p.symbol_ref.text);
                if (sym) {
                    if (sym->isskip()) {
                        error_count += 1;
                        std::cerr << "alternative is using the skip terminal '" <<
                            sym->name() <<"'\n";
                    }
                    std::optional<std::string_view> alias = std::nullopt;
                    if (p.alias) {
                        alias = p.alias->text;
                    }
                    s.emplace_back(*sym, alias);
                } else if (p.symbol_ref.text[0] == '\'') {
                    // add an inline terminal
                    // We'll need to come up with a nice name.
                    out.atoms.emplace_back("_TERM"s + std::to_string(++inline_terminal_count));
                    terminal_symbol t;
                    t.name = out.atoms.back();
                    t.pattern = p.symbol_ref.text;
                    t.type_str = "void"sv;
                    t.is_inline = true;
                    auto [ inserted, new_sym ] = out.symbols.add(t.name, t);
                    assert(inserted);
                    out.symbols.register_key(t.pattern, new_sym);
                    s.emplace_back(new_sym, std::nullopt);

                } else {
                    error_count += 1;
                    std::cerr << "alternative requires grammar symbol '" <<
                        p.symbol_ref.text << "' that does not exist.\n";
                }
            }

            out.productions.emplace_back(
                    production_identifier_t::get_next_id(), 
                    *rsym, (alt.action ? (*alt.action).text : ""sv), 
                    std::move(s));
        }
    }

    template<typename T>
    void operator()(const T & _) {
        /* don't care about terminals this time */
    }

};

std::unique_ptr<yalr::analyzer_tree> analyze(const yalr::parse_tree &tree) {
    auto retval = std::make_unique<yalr::analyzer_tree>();

    retval->options.init_defaults(option_defaults);

    int error_count = 0;

    /* sort the defs out into terms and rules
     * check to make sure that names are unique
     */
    phase_i_visitor sv{*retval};

    for (auto &d : tree.statements) {
        std::visit(sv, d);
    }

    error_count += sv.error_count;

    // Make sure there is a goal defined
    if (not sv.goal_rule) {
        std::cerr << "No goal rule was declared.\n";
        error_count += 1;
    }

    if (error_count > 0) {
        std::cerr << "Errors discovered. Halting.\n";
        return nullptr;
    }

    /* Now, iterate through the rules and alts and
     * check that all the items are defined.
     * Create the map of productions at the same time.
     */
    prod_visitor pv{*retval};
    for (const auto &d : tree.statements) {
        std::visit(pv, d);
    }

    error_count += pv.error_count;

    /* As a last step, augment the grammar with a "Rule 0" that
     * is simply : Goal' => Goal
     * This will act as our true goal.
     */

    // add the rule.
    retval->atoms.emplace_back(std::string(sv.goal_rule->name.text) + "_prime");

    rule_symbol aug_rule;
    aug_rule.name = retval->atoms.back();
    aug_rule.type_str = "void";

    auto [ added, new_sym ] = retval->symbols.add(aug_rule.name, aug_rule);

    assert(added);

    // add the production.
    std::vector<yalr::prod_item>ts;
    ts.emplace_back(*(retval->symbols.find(sv.goal_rule->name.text)), "");

    retval->productions.emplace_back(
            production_identifier_t::get_next_id(), 
            new_sym, ""sv, std::move(ts));

    retval->target_prod = retval->productions.back().prod_id;

    // add the pseudo terminal '$' to represent the
    // end of input.
    terminal_symbol eoi;
    eoi.name = "$";
    eoi.type_str = "void";
    retval->symbols.add(eoi.name, eoi);

    if (error_count > 0) {
        std::cerr << "Errors discovered. Halting.\n";
        return nullptr;
    }

    return retval;
}


void pretty_print(const analyzer_tree &tree, std::ostream& strm) {

    strm << "----- BEGIN SYMBOLS -----\n";
    for (const auto &[key, value] : tree.symbols) {
        strm << "  " << key << ": " << value.type_name() << 
            ' ' << value.name() << "\n";
        switch (value.type()) {
            case symbol_type::skip :
                {
                    auto info = value.get_data<symbol_type::skip>();
                    strm << "      pattern = " << info->pattern << "\n";
                }
                break;
            case symbol_type::terminal :
                {
                    auto info = value.get_data<symbol_type::terminal>();
                    if (info->is_inline) {
                        strm << "      defined inline" << "\n";
                    }
                    strm << "      pattern = " << info->pattern << "\n";
                    // strm << "      alias   = " << info->alias << "\n";
                    strm << "      type    = " << info->type_str << "\n";
                }
                break;
            case symbol_type::rule :
                {
                    auto info = value.get_data<symbol_type::rule>();
                    if (info->isgoal) {
                        strm << "      goal" << "\n";
                    }
                    strm << "      type    = " << info->type_str << "\n";
                }
                break;
            default :
                break;
        }
    }
    strm << "----- END SYMBOLS -----\n";

    strm << "----- BEGIN ATOM LIST -----\n";
    for (const auto &a : tree.atoms) {
        strm << "   '" << a << "'\n";
    }
    strm << "----- END ATOM LIST -----\n";
}

} // namespace yalr::analyzer
