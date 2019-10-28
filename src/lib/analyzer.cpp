#include "analyzer.hpp"

#include "yassert.hpp"

#include <unordered_set>


namespace yalr {

std::unordered_set<std::string_view> verbatim_locations = {
    { "file.top" },      {"file.bottom"},
    { "namespace.top" }, {"namespace.bottom"},
    { "lexer.top" },     {"lexer.bottom"},
    { "parser.top" },    {"parser.bottom"},
};

namespace analyzer {
//
// Helper function to register a pattern as a new terminal
//
symbol register_pattern_terminal(analyzer_tree& out, text_fragment pattern);

//
// Pattern helper
//
template<class T>
void fix_up_pattern(T& x, optional_text_fragment case_text, analyzer_tree& out) {
    case_type ct = case_type::undef;

    if (case_text) {
        if (case_text->text == "@cmatch") {
            ct = case_type::match;
        } else if (case_text->text == "@cfold") {
            ct = case_type::fold;
        } else {
            // parser did something wrong.
            yfail("Invalid pattern case specifier");
        }
    }
    auto full_pattern = x.pattern;
    if (x.pattern[0] == '\'') {
        x.pattern = x.pattern.substr(1, full_pattern.size()-2);
        x.pat_type = pattern_type::string;
        // This will have to reference the option once
        // that is in place
        if (ct == case_type::undef) {
            x.case_match = out.options.lexer_case.get();
        } else {
            x.case_match = ct;
        }

    } else if (!x.pattern.compare(0, 3, "rf:")) {
        x.pattern = x.pattern.substr(3, full_pattern.size()-2);
        x.pat_type = pattern_type::regex;
        if (ct == case_type::undef) {
            x.case_match = case_type::fold;
        } else {
            out.record_error(*case_text, "multiple case specifiers");
        }
    } else if (!x.pattern.compare(0, 3, "rm:")) {
        x.pattern = x.pattern.substr(3, full_pattern.size()-2);
        x.pat_type = pattern_type::regex;
        if (ct == case_type::undef) {
            x.case_match = out.options.lexer_case.get();
        } else {
            out.record_error(*case_text, "multiple case specifiers");
        }
    } else if (!x.pattern.compare(0, 2, "r:")) {
        x.pattern = x.pattern.substr(2, full_pattern.size()-1);
        x.pat_type = pattern_type::regex;
        // This will have to reference the option once
        // that is in place
        if (ct == case_type::undef) {
            x.case_match = case_type::match;
        } else {
            x.case_match = ct;
        }
    } else {
        yfail("Could not deduce pattern type from string start");
    }
}

//
// Helper function to parse an associativity specifier
//
assoc_type parse_assoc(analyzer_tree& out, text_fragment spec) {
    if (spec.text == "left") {
        return assoc_type::left;
    }

    if (spec.text == "right") {
        return assoc_type::right;
    }

    out.record_error("Invalid associativity specifier", spec);

    return assoc_type::undef;
}



//
// Helper function used to dereference a precedence specifier.
//
std::optional<int> parse_precedence(analyzer_tree& out, text_fragment spec) {
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
    analyzer_tree &out;
    int rule_count = 0;

    std::optional<rule_stmt> goal_rule = std::nullopt;

    explicit phase_i_visitor(analyzer_tree& g_) : out(g_) {};


    //
    // Handle rules.
    // Add the rule name to the symbol table, keep track of the
    // goal rule. Make sure that no rule has been defined twice.
    // (may need to loosen this to allow the addition of productions
    // to a rule)
    //
    void operator()(const rule_stmt &r) {
        auto rs = rule_symbol{r};
        auto [ inserted, new_sym ] = out.symbols.add(r.name.text, rs);

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

        if (r.type_str) {
            if (r.type_str->text == "@lexeme") {
                rs.type_str = "std::string";
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
    void operator()(const terminal_stmt &t) {

        terminal_symbol ts{t};

        if (t.type_str) {
            if (t.type_str->text == "@lexeme") {
                if (t.action) {
                    out.record_error(t.name, "terminal has type @lexeme but "
                            "already has an action");
                } else {
                    ts.action = "return std::move(lexeme);";
                    ts.type_str = "std::string";
                }
            } else if (t.type_str->text != "void" and not t.action) {
                out.record_error(t.name, "'", t.name,
                    "' has non-void type but no action to assign a "
                    " value");
            }
        } else {
            ts.type_str = "void"sv;
        }

        // associativity

        if (t.associativity) {
            ts.associativity =  parse_assoc(out, *t.associativity);
        }

        // precedence
        if (t.precedence) {
            ts.precedence = parse_precedence(out, *t.precedence);
        }

        // pattern type
        std::string_view full_pattern = ts.pattern;
        ts.case_match = case_type::match;

        fix_up_pattern(ts, t.case_match, out);


        ts.is_inline = t.is_inline;

        if (ts.is_inline) {
            //
            // all the is_inline stuff should be coming from (e.g.) rule code
            // that has already checked that the symbol isn't there.
            // So, we don't have to be quite as picky here.
            //
            yassert (ts.pat_type == pattern_type::string, "inline pattern must be a fixed string pattern");
            out.atoms.emplace_back("0TERM"s + std::to_string(out.atoms.size()+1));
            ts.token_name = out.atoms.back();
            ts.name = full_pattern;
            auto [ inserted, new_sym ] = out.symbols.add(ts.name, ts);
            yassert(inserted, "Failed to insert new inline symbol");

        } else {
            ts.token_name = ts.name;
            // insert the name
            auto [ inserted, new_sym ] = out.symbols.add(ts.name, ts);

            if (! inserted ) {
                out.record_error(t.name, "symbol"
                    "' has already been defined as a ",
                    new_sym.type_name());
            }

            // insert the pattern (maybe)
            if (ts.pat_type == pattern_type::string) {
                auto [ inserted, pattsym ] = out.symbols.register_key(full_pattern, new_sym);

                if (! inserted ) {
                    out.record_error(t.pattern, "pattern '", full_pattern,
                        "' has already been defined by ",
                        pattsym.type_name(), " ", pattsym.name());
                }
            }
        }
    }

    //
    // Handle skips.
    // Add the name and (maybe) pattern to the symbol table.
    // Check for name clashes.
    //
    void operator()(const skip_stmt &s) {
        skip_symbol ss(s);

        if (s.associativity) {
            out.record_error(*s.associativity,
                    "skip terminals may not have associativity specified"
                    );
        }
        if (s.precedence) {
            out.record_error(*s.precedence,
                    "skip terminals may not have precedence specified"
                    );
        }
        if (s.action) {
            out.record_error(*s.action,
                    "skip terminals may not have an action defined"
                    );
        }
        std::string_view full_pattern = ss.pattern;
        fix_up_pattern(ss, s.case_match, out);
        ss.token_name = ss.name;


        auto [ inserted, new_sym ] = out.symbols.add(ss.name, ss);

        if (! inserted ) {
            out.record_error(s.name, "'" , ss.name ,
                "' has already been defined as a " ,
                new_sym.type_name());
        }
        // insert the pattern (maybe)
        if (ss.pat_type == pattern_type::string) {
            auto [ inserted, pattsym ] = out.symbols.register_key(full_pattern, new_sym);

            if (! inserted ) {
                out.record_error(s.name, "pattern '" , full_pattern,
                    "' has already been defined by ",
                    pattsym.type_name() , " ", pattsym.name());
            }
        }
    }

    //
    // Handle options.
    //
    void operator()(const option_stmt &t) {
        if (out.options.contains(t.name.text)) {
            if (not out.options.validate(std::string(t.name.text), t.setting.text)) {
                out.record_error(t.name, "option '", t.name,
                    "' has already be set");
            }
        } else {
            out.record_error(t.name, "Unknown option '", t.name, "'");
        }

    }

    //
    //// Handle verbatim.
    //
    void operator()(const verbatim_stmt &t) {
        if (verbatim_locations.count(t.location.text) > 0) {
            out.verbatim_map.emplace(t.location.text, t.text.text);
        } else {
            out.record_error("Unknown location for verbatim section", t.location);
        }
    }

    //
    //// Handle precedence
    //
    void operator()(const precedence_stmt &t) {
        auto precedence = parse_precedence(out, t.prec_ref);
        for (auto const& i : t.symbol_refs) {
            auto sym = out.symbols.find(i.text);
            if (sym) {
                if (sym->isterm()) {
                    auto data = sym->get_data<symbol_type::terminal>();
                    if (data->precedence) {
                        out.record_error("terminal already has precedence set", i);
                    } else {
                        data->precedence = precedence;
                    }
                } else {
                    out.record_error("symbol reference is not a terminal", i);
                }
            } else if (i.text[0] == '\'') {
                auto new_sym = register_pattern_terminal(out, i);
                auto data = new_sym.get_data<symbol_type::terminal>();
                data->precedence = precedence;
            } else {
                out.record_error("Unknown symbol used in statement", i);
            }
        }

    }

    //
    //// Handle associativity
    //
    void operator()(const associativity_stmt &t) {
        assoc_type associativity = parse_assoc(out, t.assoc_text);

        for (auto const& i : t.symbol_refs) {
            auto sym = out.symbols.find(i.text);
            if (sym) {
                if (sym->isterm()) {
                    auto data = sym->get_data<symbol_type::terminal>();
                    if (data->associativity != assoc_type::undef) {
                        out.record_error("terminal already has associativity set", i);
                    } else {
                        data->associativity = associativity;
                    }
                } else {
                    out.record_error("symbol reference is not a terminal", i);
                }
            } else if (i.text[0] == '\'') {
                auto new_sym = register_pattern_terminal(out, i);
                auto data = new_sym.get_data<symbol_type::terminal>();
                data->associativity = associativity;
            } else {
                out.record_error("Unknown symbol used in statement", i);
            }
        }
    }

    //
    //// Handle termset
    //
    void operator()(const termset_stmt &t) {
        bool has_assoc = false;
        assoc_type assoc;

        if (t.associativity) {
            has_assoc = true;
            assoc = parse_assoc(out, *t.associativity);
        }

        bool has_prec = false;
        std::optional<int> prec;
        if (t.precedence) {
            has_prec = true;
            prec = parse_precedence(out, *t.precedence);
        }

        std::vector<symbol>symlist;

        for (auto const& i : t.symbol_refs) {
            auto sym = out.symbols.find(i.text);
            if (sym) {
                if (sym->isterm()) {
                    auto data = sym->get_data<symbol_type::terminal>();
                    if (has_assoc) {
                        if (data->associativity != assoc_type::undef) {
                            out.record_error(
                                    "terminal already has associativity set",
                                    i);
                        } else {
                            data->associativity = assoc;
                        }
                    }
                    if (has_prec) {
                        if (data->precedence) {
                            out.record_error(
                                    "terminal already has precedence set", i);
                        } else {
                            data->precedence = prec;
                        }
                    }
                } else {
                    out.record_error("symbol reference is not a terminal", i);
                }
            } else if (i.text[0] == '\'') {
                auto new_sym = register_pattern_terminal(out, i);
                auto data = new_sym.get_data<symbol_type::terminal>();
                if (has_assoc) {
                    data->associativity = assoc;
                }

                if (has_prec) {
                    data->precedence =prec;
                }
            } else {
                out.record_error("Unknown symbol used in statement", i);
            }
        }


    }

};

//
// Phase II visitor
//
// Focused on the rules. Link the items from the rule_defs to actual symbols.
//
struct phase_ii_visitor {
    analyzer_tree &out;
    int inline_terminal_count = 0;

    explicit phase_ii_visitor(yalr::analyzer_tree &g_) : out(g_) {};

    void operator()(const rule_stmt &r) {
        /* run through each alternative and generate a production.
         * Make sure every referenced item exists.
         */
        auto rsym = out.symbols.find(r.name.text);
        yassert(rsym, "Could not find symbol for rule in Phase 2"); // if not found, something went wrong up above.

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
                    auto new_sym = register_pattern_terminal(out, p.symbol_ref);
                    s.emplace_back(new_sym, std::nullopt);
                    implied_precedence = new_sym.get_data<symbol_type::terminal>()->precedence;

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

//
//
//
symbol register_pattern_terminal(yalr::analyzer_tree& out, text_fragment pattern) {

    terminal_stmt t;
    t.pattern = pattern;
    t.is_inline = true;

    phase_i_visitor sv{out};

    sv(t);

    auto sym = out.symbols.find(pattern.text);
    yassert(sym, "Could not find symbol just register for inline terminal");

    return *sym;

}

std::unique_ptr<yalr::analyzer_tree> analyze(const yalr::parse_tree &tree) {
    auto retval = std::make_unique<yalr::analyzer_tree>();


    /* sort the defs out into terms and rules
     * check to make sure that names are unique
     */
    phase_i_visitor sv{*retval};

    for (auto &d : tree.statements) {
        std::visit(sv, d);
    }


    // Make sure there is a goal defined
    if (not sv.goal_rule) {
        retval->record_error("No goal rule was declared.", text_fragment{"", text_location{0, tree.source}});
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

    yassert(added, "Could not add synthetic goal rule");

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
                            yfail("Invalid association type while printing");
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
            strm << " " << i.sym.name();
        }

        strm << "\n";

    }
    strm << "----- END PRODUCTIONS --------\n";
}

}
} // namespace yalr::analyzer
