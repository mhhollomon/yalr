#include "analyzer.hpp"

#include <variant>

namespace yalr::analyzer {

int terminal::next_value = -1;
int production::next_value = -1;

struct sorting_visitor {
    grammar &out;
    int error_count = 0;
    int rule_count = 0;
    explicit sorting_visitor(grammar &g_) : out(g_) {};

    void operator()(const ast::rule_def &r) {
        auto [ inserted, new_sym ] = out.syms.insert(r);

        if (! inserted ) {
            std::cerr << "'" << r.name << 
                "' has already been defined as a " <<
                new_sym.type_name() << "\n";
            error_count += 1;
        }

        if (r.isgoal) {
            if (out.goal.empty()) {
                std::cerr << "'" << r.name <<
                    "' is marked as a goal rule, but '" << 
                    out.goal << "' is already marked\n";
                error_count += 1;
            } else {
                out.goal = r.name;
            }
        }
    }

    void operator()(const ast::terminal &t) {
        auto [ inserted, new_sym ] = out.syms.insert(t);

        if (! inserted ) {
            std::cerr << "'" << t.name << 
                "' has already been defined as a " <<
                new_sym.type_name() << "\n";
            error_count += 1;
        }

    }
};

struct prod_visitor {
    grammar &out;
    int error_count = 0;
    explicit prod_visitor(grammar &g_) : out(g_) {};

    void operator()(const ast::rule_def &r) {
        /* run through each alternative and generate a production.
         * Make sure every referenced item exists.
         */
        auto [rfound, rsym] = out.syms.find(r.name);
        //NOLINTNEXTLINE
        assert(rfound); // if not found, something went wrong up above.

        for (const auto& a : r.alts) {
            std::vector<SymbolTable::symbol> s;
            for (const auto& p : a.pieces) {
                auto [found, sym] = out.syms.find(p);
                if (found) {
                    s.push_back(sym);
                } else {
                    error_count += 1;
                    std::cerr << "alternative requires gammar symbol '" <<
                        p << "' that does not exist.\n";
                }
            }

            out.productions.emplace_back(rsym, std::move(s));
        }
    }

    void operator()(const ast::terminal & _) {
        /* don't care about terminals this time */
    }

};

std::unique_ptr<grammar> analyze(const parser::ast_tree_type &tree) {
    int error_count = 0;
    auto retval = std::make_unique<grammar>();

    if (tree.parser_class.has_value()) {
        retval->parser_class = *tree.parser_class;
    } else {
        retval->parser_class = "YalrParser";
    }

    /* sort the defs out into terms and rules
     * check to make sure that names are unique
     */
    sorting_visitor sv{*retval};

    for (const auto &d : tree.defs) {
        std::visit(sv, d);
    }

    error_count += sv.error_count;

    // Make sure there is a goal defined
    if (retval->goal.empty()) {
        std::cerr << "No goal rule was declared.\n";
        error_count += 1;
    }

    if (error_count > 0) {
        std::cerr << "Errors discovered. Halting\n";
        return nullptr;
    }

    /* Now, iterate through the rules and alts and
     * check that all the items are defined.
     * Create the map of productions at the same time.
     */
    prod_visitor pv{*retval};
    for (const auto &d : tree.defs) {
        std::visit(pv, d);
    }

    error_count += pv.error_count;

    /* As a last step, augment the grammar with a "Rule 0" that
     * is simply : Goal' => Goal
     * This will act as our true goal.
     */

    // add the rule.
    std::string aug_rule_name = retval->goal + "_prime";

    ast::rule_def aug_rule;
    aug_rule.isgoal = false;
    aug_rule.name = aug_rule_name;

    auto [ _, new_sym ] = retval->syms.insert(aug_rule);

    // add the production.
    std::vector<SymbolTable::symbol>ts;
    ts.push_back(retval->syms.find(retval->goal).second);

    retval->productions.emplace_back(new_sym, std::move(ts));

    retval->target_prod = retval->productions.size()-1;

    // add the psuedo terminal '$' to represent the
    // end of input.
    ast::terminal eoi;
    eoi.name = "$";
    retval->syms.insert(eoi);

    if (error_count > 0) {
        std::cerr << "Errors discovered. Halting\n";
        return nullptr;
    }

    return retval;
}


void pp::operator()(const grammar &g) {
    strm << "class " << g.parser_class << " {};\n";

    strm << "Symbols ==\n";
    for (const auto &iter : g.syms) {
        auto &s = iter.second;
        strm << "  " << s.name() << " (" << 
            s.type_name() << " " << s.id() << ")" << "\n";
    }

    strm << "GOAL == " << g.goal << "\n";

    strm << "PRODUCTIONS ==\n";

    for (const auto& p : g.productions) {
        strm << "  ";
        (*this)(p);
        strm << ";\n";
    }

    strm << "TARGET == " << g.target_prod << "\n";
}

void pp::operator()(const production& p) {
    strm << "[" << p.prod_id << "] " << 
        p.rule.name() << "(" << p.rule.id() << ") =>";
    for (const auto& s : p.syms) {
        strm << " " << s.name() << "(" << s.id() << ")";
    }
}

void pretty_print(const grammar &g, std::ostream& strm) {
    pp mypp{strm};
    
    mypp(g);
}

} // namespace yalr::analyzer
