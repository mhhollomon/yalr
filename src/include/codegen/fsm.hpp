#pragma once

#include "utils.hpp"
#include "symbols.hpp"
#include <map>
#include <variant>

namespace yalr::codegen {

    struct nfa_state_id : util::identifier_t<nfa_state_id> {
        using util::identifier_t<nfa_state_id>::identifier_t;
    };

    struct nfa_state  {
        nfa_state_id id;
        bool accepting = false;
        symbol_identifier_t accepted_symbol;

        using input_symbol = std::variant<std::monostate, char>;
        std::multimap<input_symbol, nfa_state_id> transitions_;
    };

    struct nfa_machine {
        std::map<nfa_state_id, nfa_state> states_;
        nfa_state_id start_state_;
        std::set<nfa_state_id> accepting_states_;

        // These modify the base machine
        nfa_machine &union_in(const nfa_machine &o);
        nfa_machine &concat_in(const nfa_machine &o);
        nfa_machine &close_in(const nfa_machine &o);

        static std::unique_ptr<nfa_machine> build_from_string(std::string_view input,
                symbol_identifier_t sym_id);
    };

    /*=================================================================*/
    struct dfa_state_id : util::identifier_t<dfa_state_id> {
        using util::identifier_t<dfa_state_id>::identifier_t;
    };

    struct dfa_state  {
        dfa_state_id id;
        bool accepting = false;
        // the dfa may accept multiple symbols in the same
        // state. The decision as to which gets returned is
        // going to depend on which were passed as valid
        // (context aware lexing), length of the accepted string
        // relative position in the original grammar (early wins).
        std::set<symbol_identifier_t> accepted_symbol;

        using input_symbol = char;
        std::map<input_symbol, dfa_state_id> transitions_;
    };

    struct dfa_machine {
        std::map<dfa_state_id, dfa_state> states_;
        dfa_state_id start_state_;
        std::set<dfa_state_id> accepting_states_;

    };
} // yalr
