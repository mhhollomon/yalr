#include "analyzer.hpp"

namespace yalr { namespace analyzer {

int terminal::next_value = -1;

struct sorting_visitor {
    grammar &out;
    int error_count = 0;
    sorting_visitor(grammar &g_) : out(g_) {};

    void operator()(const ast::rule_def &r) {
        if (out.terms.count(r.name) > 0) {
            std::cerr << "'" << r.name << "' has already been defined as a terminal\n";
            error_count += 1;
            return;
        }
        if (out.rules.count(r.name) > 0) {
            std::cerr << "'" << r.name << "' has already been defined as a rule\n";
            error_count += 1;
            return;
        }

        out.rules.insert(std::make_pair(r.name, rule_def{r}));

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
        if (out.rules.count(t.name) > 0) {
            std::cerr << "'" << t.name << "' has already been defined as a rule\n";
            error_count += 1;
            return;
        }
        out.terms.insert(std::make_pair(t.name, terminal{t.name}));
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

    /* Now, iterate through the rules and alts and
     * check that all the items are defined.
     */

    for (const auto &mappair : retval->rules) {
        const auto &rule = mappair.second;
        for (const auto &alt : rule.alts) {
            for (const auto &i : alt.pieces) {
                if ( (retval->rules.count(i) == 0) && 
                        (retval->terms.count(i) == 0)) {
                    error_count += 1;
                    std::cerr << "item '" << i 
                        << "' not defined in rule " << rule.name << "\n";
                }
            }
        }
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
        for (const auto &p : g.rules) {
            strm << "  " << p.first << "\n";
        }

        strm << "GOAL == " << g.goal << "\n";
    }
};

void pretty_print(const grammar &g, std::ostream& strm) {
    pp mypp{strm};
    
    mypp(g);
}

}}
