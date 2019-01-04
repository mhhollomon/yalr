#include "analyzer.hpp"

#include <variant>

namespace yalr { namespace analyzer {

int terminal::next_value = -1;
int production::next_value = -1;

struct sorting_visitor {
    grammar &out;
    int error_count = 0;
    int rule_count = 0;
    sorting_visitor(grammar &g_) : out(g_) {};

    void operator()(const ast::rule_def &r) {
        if (out.terms.count(r.name) > 0) {
            std::cerr << "'" << r.name << "' has already been defined as a terminal\n";
            error_count += 1;
            return;
        }
        if (out.rules_by_name.count(r.name) > 0) {
            std::cerr << "'" << r.name << "' has already been defined as a rule\n";
            error_count += 1;
            return;
        }

        int rule_num = ++rule_count;

        out.rules_by_name.insert(std::make_pair(r.name, rule_num));
        out.rules_by_id.insert(std::make_pair(rule_num, r.name));

        if (r.isgoal) {
            if (out.goal != "") {
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
        if (out.terms.count(t.name) > 0) {
            std::cerr << "'" << t.name << "' has already been defined as a terminal\n";
            error_count += 1;
            return;
        }
        if (out.rules_by_name.count(t.name) > 0) {
            std::cerr << "'" << t.name << "' has already been defined as a rule\n";
            error_count += 1;
            return;
        }
        out.terms.insert(std::make_pair(t.name, terminal{t.name}));
    }
};

struct prod_visitor {
    grammar &out;
    int error_count = 0;
    prod_visitor(grammar &g_) : out(g_) {};

    void operator()(const ast::rule_def &r) {
        /* run through each alternative and generate a production.
         * Make sure every referenced item exists.
         */
        for (const auto& a : r.alts) {
            std::vector<symbol> s;
            for (const auto& p : a.pieces) {
                auto rule_iter = out.rules_by_name.find(p);
                if (rule_iter != out.rules_by_name.end()) {
                    // we have a rule
                    s.emplace_back(symbol::ruleSymbol, rule_iter->second);
                } else {
                    auto term_iter  = out.terms.find(p);
                    // earlier processing made sure things were defined.
                    assert(term_iter != out.terms.end());

                    s.emplace_back(symbol::termSymbol, term_iter->second.value);
                }
            }

            auto rule_id = out.rules_by_name.find(r.name)->second;
            out.productions.emplace_back(rule_id, std::move(s));
        }
    }

    void operator()(const ast::terminal &) {
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
    if (retval->goal == "") {
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
    std::string aug_rule = retval->goal + "_prime";
    retval->rules_by_name.insert(std::make_pair(aug_rule, 0));
    retval->rules_by_id.insert(std::make_pair(0, aug_rule));

    // add the production.
    std::vector<symbol>ts;
    int goal_rule_id = retval->rules_by_name.find(retval->goal)->second;
    ts.emplace_back(symbol::ruleSymbol, goal_rule_id);

    retval->productions.emplace_back(0, std::move(ts));

    if (error_count > 0) {
        std::cerr << "Errors discovered. Halting\n";
        return nullptr;
    }

    return retval;
}


struct pp {
    std::ostream& strm;

    void operator()(const grammar &g) {
        strm << "class " << g.parser_class << " {};\n";

        strm << "TERMS ==\n";
        for (const auto &p : g.terms) {
            auto &t = p.second;
            strm << "  " << t.name << " (" << t.value << ")" << "\n";
        }

        strm << "RULES ==\n";
        for (const auto &r : g.rules_by_name) {
            strm << "  " << r.first << "(" << r.second << ")\n";
        }

        strm << "GOAL == " << g.goal << "\n";

        strm << "PRODUCTIONS ==\n";

        for (const auto& p : g.productions) {
            strm << "  [" << p.prod_id << "] Rule " << p.rule_id << " =>";
            for (const auto& s : p.syms) {
                strm << " " << s.type_name() << " " << s.symbol_id;
            }
            strm << ";\n";
        }
    }
};

void pretty_print(const grammar &g, std::ostream& strm) {
    pp mypp{strm};
    
    mypp(g);
}

}}
