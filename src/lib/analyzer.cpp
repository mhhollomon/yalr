#include "analyzer.hpp"

#include "option_defaults.hpp"

namespace yalr::analyzer {

//
// Helper function used to dereference a precedence specifier.
//
std::optional<int> parse_precedence(yalr::analyzer_tree& out, text_fragment spec) {
    if (spec.text[0] == '-') {
        out.record_error("precedence cannot be negative", spec);
    } else if (std::isdigit(spec.text[0])) {
        return atoi(std::string(spec.text).data());
    } else {
        auto sym_ref = out.symbols.find(spec.text);
        if (sym_ref) {
            if (sym_ref->isterm()) {
                auto prec = sym_ref->get_data<symbol_type::terminal>()->precedence;
                if (prec) {
                    return prec;
                } else {
                    out.record_error(
                            "Referenced symbol does not have precedence set",
                            spec); 
                }
            } else {
                out.record_error(
                        "Referenced symbol is not a terminal",
                        spec);
            }
        } else {
            out.record_error(
                    "Undefined terminal used for precedence",
                    spec);
        }
    }
    return std::nullopt;
}
//
// Phase I visitor
//
// This pass is basically just to build the symbol table. Once we've got all
// the symbols we can do other things.
//
struct phase_i_visitor {
    yalr::analyzer_tree &out;
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
            out.record_error(r.name, "'", r.name.text, 
                    "' has already been defined as a ",
                    new_sym.type_name());
        }

        if (r.isgoal) {
            if (goal_rule) {
                out.record_error(r.name, 
                        "'" , r.name.text ,
                        "' is marked as a goal rule, but '",
                        goal_rule->name.text , "' is already marked");
            } else {
                goal_rule = r;
            }
        }
    }

    //
    // Handle Terminals.
    // Add the name and the pattern(if string and not regex) to
    // the symbol table. Check that an action has been given if the
    // term has a non-void type. Check for dups. Fill out the assoc 
    // and precedence.
    //
    void operator()(const yalr::terminal &t) {

        terminal_symbol ts{t};

        if (t.type_str) {
            if (t.type_str->text != "void" and not t.action) {
                out.record_error(t.name, "'", t.name,
                    "' has non-void type but no action to assign a "
                    " value");
            }
        } else {
            ts.type_str = "void"sv;
        }

        // associativity

        if (t.associativity) {
            if (t.associativity->text == "left") {
                ts.associativity = assoc_type::left;
            } else if (t.associativity->text == "right") {
                ts.associativity = assoc_type::right;
            } else {
                out.record_error("invalid associativity", *t.associativity);
            }
        }

        // precedence
        if (t.precedence) {
            ts.precedence = parse_precedence(out, *t.precedence);
        }


        // insert the name
        auto [ inserted, new_sym ] = out.symbols.add(ts.name, ts);

        if (! inserted ) {
            out.record_error(t.name, "symbol"
                "' has already been defined as a ",
                new_sym.type_name());
        }

        // insert the pattern (maybe)
        if (ts.pattern[0] == '\'') {
            auto [ inserted, pattsym ] = out.symbols.register_key(ts.pattern, new_sym);

            if (! inserted ) {
                out.record_error(t.pattern, "pattern '", ts.pattern,
                    "' has already been defined by ",
                    pattsym.type_name(), " ", pattsym.name());
            }
        }



    }

    //
    // Handle skips.
    // Add the name and (maybe) pattern to the symbol table.
    // Check for name clashes.
    //
    void operator()(const yalr::skip &s) {
        skip_symbol ss(s);
        auto [ inserted, new_sym ] = out.symbols.add(ss.name, ss);

        if (! inserted ) {
            out.record_error(s.name, "'" , ss.name ,
                "' has already been defined as a " ,
                new_sym.type_name());
        }
        // insert the pattern (maybe)
        if (ss.pattern[0] == '\'') {
            auto [ inserted, pattsym ] = out.symbols.register_key(ss.pattern, new_sym);

            if (! inserted ) {
                out.record_error(s.name, "pattern '" , ss.pattern,
                    "' has already been defined by ",
                    pattsym.type_name() , " ", pattsym.name());
            }
        }
    }

    //
    // Handle options.
    //
    void operator()(const yalr::option &t) {
        if (out.options.contains(t.name.text)) {
            if (not out.options.set_option(t.name.text, t.setting)) {
                out.record_error(t.name, "option '", t.name,
                    "' has already be set");
            }
        } else {
            out.record_error(t.name,"Unknown option '", t.name, "'");
        }

    }

};

//
// Phase II visitor
//
// Focused on the rules. Link the items from the rule_defs to actual symbols.
//
struct phase_ii_visitor {
    yalr::analyzer_tree &out;
    int inline_terminal_count = 0;

    explicit phase_ii_visitor(yalr::analyzer_tree &g_) : out(g_) {};

    void operator()(const yalr::rule &r) {
        /* run through each alternative and generate a production.
         * Make sure every referenced item exists.
         */
        auto rsym = out.symbols.find(r.name.text);
        //NOLINTNEXTLINE
        assert(rsym); // if not found, something went wrong up above.

        for (const auto& alt : r.alternatives) {
            std::vector<yalr::prod_item> s;
            std::optional<int> implied_precedence;
            for (const auto& p : alt.items) {
                auto sym = out.symbols.find(p.symbol_ref.text);
                if (sym) {
                    if (sym->isskip()) {
                        out.record_error("alternative is using a skip terminal",
                                p.symbol_ref);
                    } else if ((not alt.precedence) and sym->isterm()) {
                        //
                        // keep track of the precedence of the right-most terminal
                        // we'll using at the rules precedence if the user hasn't
                        // explicitly set it for the production.
                        //
                        auto term_ptr = sym->get_data<symbol_type::terminal>();
                        implied_precedence = term_ptr->precedence;
                    }

                    std::optional<std::string_view> alias = std::nullopt;
                    if (p.alias) {
                        alias = p.alias->text;
                    }
                    s.emplace_back(*sym, alias);
                } else if (p.symbol_ref.text[0] == '\'') {
                    // add an inline terminal
                    // We'll need to come up with a nice name.
                    out.atoms.emplace_back("0TERM"s + std::to_string(++inline_terminal_count));
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
                    out.record_error(p.symbol_ref, 
                        "alternative requires grammar symbol that does not exist");
                }
            }


            auto &iter = out.productions.emplace_back(
                    production_identifier_t::get_next_id(), 
                    *rsym, (alt.action ? (*alt.action).text : ""sv), 
                    std::move(s));
            if (alt.precedence) {
                iter.precedence = parse_precedence(out, *alt.precedence);
            } else {
                iter.precedence = implied_precedence;
            }
                
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


    /* sort the defs out into terms and rules
     * check to make sure that names are unique
     */
    phase_i_visitor sv{*retval};

    for (auto &d : tree.statements) {
        std::visit(sv, d);
    }


    // Make sure there is a goal defined
    if (not sv.goal_rule) {
        retval->record_error("No goal rule was declared.", text_fragment{});
    }

    if (retval->errors.size() > 0) {
        retval->success = false;
        return retval;
    }

    /* Now, iterate through the rules and alts and
     * check that all the items are defined.
     * Create the map of productions at the same time.
     */
    phase_ii_visitor pv{*retval};
    for (const auto &d : tree.statements) {
        std::visit(pv, d);
    }


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

    retval->success = (retval->errors.size() == 0);

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
                    strm << "      type    = " << info->type_str << "\n";
                    strm << "      assoc   = ";
                    switch (info->associativity) {
                        case (assoc_type::undef) :
                            strm << "undef\n";
                            break;
                        case assoc_type::left :
                            strm << "left\n";
                            break;
                        case assoc_type::right :
                            strm << "right\n";
                            break;
                        default :
                            assert(false);
                            break;
                    }
                    strm << "      prec    = ";
                    if (info->precedence) {
                        strm << *(info->precedence) << "\n";
                    } else {
                        strm << "none\n";
                    }
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
    strm << "----- END SYMBOLS ------------\n";

    strm << "----- BEGIN ATOM LIST --------\n";
    for (const auto &a : tree.atoms) {
        strm << "   '" << a << "'\n";
    }
    strm << "----- END ATOM LIST ----------\n";
    strm << "----- BEGIN PRODUCTIONS ------\n";
    strm << "Target Production = " << int(tree.target_prod) << "\n";
    for (auto const& p : tree.productions) {
        strm << "[" << int(p.prod_id) << "] "
            << p.rule.name() 
            << " prec=";
        if (p.precedence)
            strm << *p.precedence;
        else
            strm << "(none)";
        strm << " =>";

        for (auto const& i : p.items) {
            strm << " " << i.sym.pretty_name();
        }

        strm << "\n";

    }
    strm << "----- END PRODUCTIONS --------\n";
}

} // namespace yalr::analyzer
