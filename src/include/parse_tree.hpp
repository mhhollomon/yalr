#if not defined(YALR_PARSE_TREE_HPP)
#define YALR_PARSE_TREE_HPP

#include "sourcetext.hpp"
#include "errorinfo.hpp"
#include <vector>
#include <variant>
#include <list>

namespace yalr {

    struct terminal_stmt {
        text_fragment          name;
        optional_text_fragment type_str;
        text_fragment          pattern;
        optional_text_fragment action;
        optional_text_fragment associativity;
        optional_text_fragment precedence;
        optional_text_fragment case_match;
        // not used by the parser. Used by the analyzer
        // when it is talking to itself. This will be
        // false for anything coming from the parser.
        bool is_inline = false;
    };

    // skip is a strict subset of terminal. The restrictions
    // will be enforced in the analyzer. But this lets both
    // the parser and the analyzer share code.
    struct skip_stmt : public terminal_stmt {
    };

    struct option_stmt {
        text_fragment name;
        text_fragment setting;
    };

    struct alt_item {
        text_fragment symbol_ref;
        optional_text_fragment alias;
    };

    struct alternative {
        std::vector<alt_item>  items;
        optional_text_fragment precedence;
        optional_text_fragment action;
    };

    struct rule_stmt {
        bool isgoal;
        text_fragment name;
        optional_text_fragment type_str;
        std::vector<alternative> alternatives;
    };

    struct verbatim_stmt {
        text_fragment location;
        text_fragment text;
    };

    struct associativity_stmt {
        text_fragment assoc_text;
        std::vector<text_fragment> symbol_refs;
    };

    struct precedence_stmt {
        text_fragment prec_ref;
        std::vector<text_fragment> symbol_refs;
    };

    struct termset_stmt {
        text_fragment name;
        optional_text_fragment type_str;
        optional_text_fragment associativity;
        optional_text_fragment precedence;
        std::vector<text_fragment> symbol_refs;
    };

    using statement = std::variant<
            terminal_stmt, skip_stmt, option_stmt,
            rule_stmt, verbatim_stmt,
            associativity_stmt, precedence_stmt, termset_stmt>;

    using statement_list = std::vector<statement>;
    //
    // The top level structure
    //
    struct parse_tree {
        bool success;
        statement_list statements;
        std::shared_ptr<text_source> source;
        error_list errors;

        operator bool() { return success; }

        std::ostream &pretty_print(std::ostream &strm) const;
    };

} // namespace yalr

#endif
