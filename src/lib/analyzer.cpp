#include "analyzer.hpp"

#include "analyzer_pass_1.hpp"
#include "analyzer_pass_2.hpp"

#include "yassert.hpp"


namespace yalr {

namespace analyzer {


std::unique_ptr<yalr::analyzer_tree> analyze(yalr::parse_tree &tree) {
    auto retval = std::make_unique<yalr::analyzer_tree>();


    /* Pass I
     *
     * sort the defs out into terms and rules
     * check to make sure that names are unique
     * Note that we actually pass the statement list to the visitor so that statements can spawn other statements.
     *
     */
    pass_i_visitor sv{*retval, tree.statements};

    for (auto &d : tree.statements) {
        std::visit(sv, d);
    }


    // Make sure there is a goal defined
    if (not sv.goal_rule) {
        retval->record_error(text_fragment{"", text_location{0, tree.source}},
                "No goal rule was declared.");
    }

    if (retval->errors.size() > 0) {
        retval->success = false;
        return retval;
    }

    /* Now, iterate through the rules and alts and
     * check that all the items are defined.
     * Create the map of productions at the same time.
     */
    pass_ii_visitor pv{*retval, tree.statements};
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

} // namespace analyzer
} // namespace yalr
