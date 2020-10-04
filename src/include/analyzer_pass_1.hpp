#if not defined(ANALYZER_PASS_1_HPP_)
#define ANALYZER_PASS_1_HPP_

namespace yalr {
namespace analyzer {


//
// Pass I visitor
//
// Add names of rules and rule-like objects to the symbol table.
// Keep track of the goal rule. 
// Check in docs/analyzer.md for more.
//
struct pass_i_visitor {
    analyzer_tree &out;
    statement_list &stmts_;

    std::optional<rule_stmt> goal_rule = std::nullopt;

    explicit pass_i_visitor(analyzer_tree& g_, statement_list &stmts) : out(g_), stmts_(stmts) {};


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
                rs.type_str = "std::string"sv;
            }
        }
    }


    //
    // Handle termset
    //
    void operator()(const termset_stmt &t) {
        auto rs = rule_symbol{};
        rs.name = t.name.text;
        if (t.type_str) {
            if (t.type_str->text == "@lexeme") {
                rs.type_str = "std::string"sv;
            } else {
                rs.type_str = t.type_str->text;
            }
        } else {
            rs.type_str = "void"sv;
        }

        auto [ inserted, new_sym ] = out.symbols.add(t.name.text, rs);

        if (! inserted ) {
            out.record_error(t.name, "'", t.name.text, 
                    "' has already been defined as a ",
                    new_sym.type_name());

        }

    }

    //
    // Ignore anything else
    //
    template<typename T>
    void operator()(const T & _) {
        /* don't care about terminals this time */
    }

};


//----------------------
} //namespace analyzer
} // namespace yalr

#endif
