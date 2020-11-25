#if not defined(ANALYZER_PASS_2_HPP_)
#define ANALYZER_PASS_2_HPP_

#include <unordered_set>
#include "yassert.hpp"

#include "regex_parse.hpp"

#include <regex>

namespace yalr {

std::unordered_set<std::string_view> verbatim_locations = {
    { "file.top" },      {"file.bottom"},
    { "namespace.top" }, {"namespace.bottom"},
    { "lexer.top" },     {"lexer.bottom"},
    { "parser.top" },    {"parser.bottom"},
};


namespace analyzer {


// Check in docs/analyzer.md for more.
struct pass_ii_visitor {
    analyzer_tree &out;
    statement_list &stmts_;

    explicit pass_ii_visitor(yalr::analyzer_tree &g_, statement_list &stmts) : out(g_), stmts_(stmts) {};

    //
    // Pattern helper
    //
    template<class T>
    void fix_up_pattern(T& x, optional_text_fragment case_text) {
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
        // If the case behavior was not specified, then default
        // to the current option setting.
        if (ct == case_type::undef) {
            x.case_match = out.options.lexer_case.get();
        } else {
            x.case_match = ct;
        }
        if (x.pattern[0] == '\'') {
            x.pattern = x.pattern.substr(1, full_pattern.size()-2);
            x.pat_type = pattern_type::string;
        } else if (!x.pattern.compare(0, 2, "r:")) {
            x.pattern = x.pattern.substr(2, full_pattern.size()-1);
            x.pat_type = pattern_type::regex;
        } else if (!x.pattern.compare(0, 2, "e:")) {
            x.pattern = x.pattern.substr(2, full_pattern.size()-1);
            x.pat_type = pattern_type::ecma;
        } else {
            yfail("Could not deduce pattern type from string start");
        }
    }
    //
    // Helper to parse regex patterns
    //
    template <class T>
    void parse_pattern(T& ts, text_location location) {
        if (ts.pat_type == pattern_type::ecma) {
            try {
                auto _ = std::regex{std::string(ts.pattern), std::regex_constants::ECMAScript};
            } catch (std::regex_error e) {
                out.record_error(text_fragment{ts.pattern, location}, 
                        "error in ECMAScript regex : " , e.what());
            }
        } else {
            ts.pat_rpn  = regex2rpn(text_fragment{ts.pattern, location}, 
                    (ts.pat_type == pattern_type::string)? rpn_mode::simple_string : rpn_mode::full_regex,
                    out.errors);
        }
    }

    //
    // Helper function to parse an associativity specifier
    //
    assoc_type parse_assoc(text_fragment spec) {
        if (spec.text == "left") {
            return assoc_type::left;
        }

        if (spec.text == "right") {
            return assoc_type::right;
        }

        out.record_error(spec, "Invalid associativity specifier");

        return assoc_type::undef;
    }

    //
    // Helper function used to dereference a precedence specifier.
    //
    std::optional<int> parse_precedence(text_fragment spec) {
        if (spec.text[0] == '-') {
            out.record_error(spec, "precedence cannot be negative");
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
                        out.record_error(spec,
                                "Referenced symbol does not have precedence set");
                    }
                } else {
                    out.record_error(spec,
                            "Referenced symbol is not a terminal");
                }
            } else {
                out.record_error(spec,
                        "Undefined terminal used for precedence");
            }
        }
        return std::nullopt;
    }
    //
    // Helper function to register a pattern as a new terminal
    //
    //
    symbol register_pattern_terminal(text_fragment pattern) {

        terminal_stmt t;
        t.pattern = pattern;
        t.is_inline = true;

        this->operator()(t);

        auto sym = out.symbols.find(pattern.text);
        yassert(sym, "Could not find symbol just registered for inline terminal");

        return *sym;

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
            ts.associativity =  parse_assoc(*t.associativity);
        }

        // precedence
        if (t.precedence) {
            ts.precedence = parse_precedence(*t.precedence);
        }

        // pattern type
        std::string_view full_pattern = ts.pattern;
        ts.case_match = case_type::match;

        fix_up_pattern(ts, t.case_match);


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

            parse_pattern(ts, t.pattern.location);

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

                if (inserted ) {
                    parse_pattern(ts, t.pattern.location);
                } else {
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
        fix_up_pattern(ss, s.case_match);
        ss.token_name = ss.name;


        auto [ inserted, new_sym ] = out.symbols.add(ss.name, ss);

        if (inserted ) {
            parse_pattern(ss, s.pattern.location);
        } else {
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
            out.record_error(t.location, "Unknown location for verbatim section");
        }
    }

    //
    //// Handle precedence
    //
    void operator()(const precedence_stmt &t) {
        auto precedence = parse_precedence(t.prec_ref);
        for (auto const& i : t.symbol_refs) {
            auto sym = out.symbols.find(i.text);
            if (sym) {
                if (sym->isterm()) {
                    auto data = sym->get_data<symbol_type::terminal>();
                    if (data->precedence) {
                        out.record_error(i, "terminal already has precedence set");
                    } else {
                        data->precedence = precedence;
                    }
                } else {
                    out.record_error(i, "symbol reference is not a terminal");
                }
            } else if (i.text[0] == '\'') {
                auto new_sym = register_pattern_terminal(i);
                auto data = new_sym.get_data<symbol_type::terminal>();
                data->precedence = precedence;
            } else {
                out.record_error(i, "Unknown symbol used in statement");
            }
        }

    }

    //
    //// Handle associativity
    //
    void operator()(const associativity_stmt &t) {
        assoc_type associativity = parse_assoc(t.assoc_text);

        for (auto const& i : t.symbol_refs) {
            auto sym = out.symbols.find(i.text);
            if (sym) {
                if (sym->isterm()) {
                    auto data = sym->get_data<symbol_type::terminal>();
                    if (data->associativity != assoc_type::undef) {
                        out.record_error(i, "terminal already has associativity set");
                    } else {
                        data->associativity = associativity;
                    }
                } else {
                    out.record_error(i, "symbol reference is not a terminal");
                }
            } else if (i.text[0] == '\'') {
                auto new_sym = register_pattern_terminal(i);
                auto data = new_sym.get_data<symbol_type::terminal>();
                data->associativity = associativity;
            } else {
                out.record_error(i, "Unknown symbol used in statement");
            }
        }
    }


    //
    //// Handle rule
    //
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
                        out.record_error(p.symbol_ref, "alternative is using a skip terminal");
                        // bail on this iteration of the loop. Code below relies on the symbol
                        // not being a skip.
                        continue;
                    }
                } else if (p.symbol_ref.text[0] == '\'') {
                    // add an inline terminal
                    sym = register_pattern_terminal(p.symbol_ref);

                } else {
                    out.record_error(p.symbol_ref,
                        "alternative requires grammar symbol that does not exist");
                    // bail on this iteration of the loop. Not much else we can check
                    // if the symbol doesn't exist.
                    continue;
                }
                    
                if ((not alt.precedence) and sym->isterm()) {
                    //
                    // keep track of the precedence of the right-most terminal
                    // we'll be using that as the rules precedence if the
                    // user hasn't explicitly set it for the production.
                    //
                    auto term_ptr = sym->get_data<symbol_type::terminal>();
                    implied_precedence = term_ptr->precedence;
                }

                std::optional<std::string_view> alias = std::nullopt;
                if (p.alias) {
                    bool void_type = false;
                    // we can only be a rule or terminal.
                    // skip was specifically checked above.
                    if (sym->isrule()) {
                        auto type_string = sym->get_data<symbol_type::rule>()->type_str;
                        void_type = (type_string == "void");
                    } else {
                        auto type_string = sym->get_data<symbol_type::terminal>()->type_str;
                        void_type = (type_string == "void");
                    }
                    if (void_type) {
                        out.record_error(p.symbol_ref, "symbol with type 'void' assigned an alias");
                    } else {
                        alias = p.alias->text;
                    }
                }
                s.emplace_back(*sym, alias);
            }


            auto &iter = out.productions.emplace_back(
                    production_identifier_t::get_next_id(),
                    *rsym, (alt.action ? (*alt.action).text : ""sv),
                    std::move(s));
            if (alt.precedence) {
                iter.precedence = parse_precedence(*alt.precedence);
            } else {
                iter.precedence = implied_precedence;
            }

        }
    }


    //
    //// Handle termset
    //
    void operator()(const termset_stmt &t) {

        auto sym = out.symbols.find(t.name.text);
        yassert(sym, "Could not find symbol for termset in Pass 2"); // if not found, something went wrong up above.
 
        bool has_assoc = false;
        assoc_type assoc;

        if (t.associativity) {
            has_assoc = true;
            assoc = parse_assoc(*t.associativity);
        }

        bool has_prec = false;
        std::optional<int> prec;
        if (t.precedence) {
            has_prec = true;
            prec = parse_precedence(*t.precedence);
        }

        rule_stmt new_rs;
        new_rs.name = t.name;
        new_rs.type_str = t.type_str;

        // The type string on the synthetic rule was handled in pass 1.
        // But we still need to check if we need to add the action to each alternative.
        bool do_action = false;
        std::string_view term_action;
        std::string_view termset_type_str = "void"sv;
        if (t.type_str) {
            do_action = true;
            if (t.type_str->text == "@lexeme"sv) {
                termset_type_str = "std::string"sv;
                term_action = "return std::move(lexeme);"sv;
            }
            if (t.action) {
                term_action = t.action->text;
            } else if (t.type_str->text != "@lexeme") {
                term_action = "return 1;" ;
                out.record_error(t.name, "typed termsets must have an action");
            }

        } else if (t.action) {
            out.record_error(t.name, "void termsets cannot have an action");
        }

        for (auto const& i : t.symbol_refs) {
            //
            // build the alternative for the synthetic rule
            //
            alternative alt;
            if (do_action) {
                alt.action = text_fragment{"return _v1;"sv, i.location};
            }

            alt.items.push_back({i, std::nullopt});
            new_rs.alternatives.push_back(alt);

            //
            // validate and update the terminal.
            //
            auto sym = out.symbols.find(i.text);
            if (sym) {
                if (sym->isterm()) {
                    auto data = sym->get_data<symbol_type::terminal>();
                    if (has_assoc) {
                        if (data->associativity != assoc_type::undef) {
                            out.record_error(i,
                                    "terminal already has associativity set");
                        } else {
                            data->associativity = assoc;
                        }
                    }
                    if (has_prec) {
                        if (data->precedence) {
                            out.record_error(i,
                                    "terminal already has precedence set");
                        } else {
                            data->precedence = prec;
                        }
                    }
                    if (termset_type_str != "void" and data->type_str != termset_type_str) {
                        out.record_error(i, "terminal value type does not match termset");
                    }
                } else {
                    out.record_error(i, "symbol reference is not a terminal");
                }
            } else if (i.text[0] == '\'') {
                auto sym = register_pattern_terminal(i);
                auto data = sym.get_data<symbol_type::terminal>();
                if (has_assoc) {
                    data->associativity = assoc;
                }

                if (has_prec) {
                    data->precedence = prec;
                }
                data->type_str = termset_type_str;
                if (do_action) {
                    data->action = term_action;
                }
            } else {
                out.record_error(i, "Unknown symbol used in statement");
            }
        } // for loop

        // Process the new rule we built
        this->operator()(new_rs);

    }



    template<typename T>
    void operator()(const T & _) {
        /* don't care about terminals this time */
    }

};

//----------------------
} //namespace analyzer
} // namespace yalr

#endif
