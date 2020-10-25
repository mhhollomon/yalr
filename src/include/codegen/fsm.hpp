#pragma once

#include "utils.hpp"
#include "symbols.hpp"
#include <map>
#include <variant>

namespace yalr::codegen {

    struct nfa_state_identifier_t : util::identifier_t<nfa_state_identifier_t> {
        using util::identifier_t<nfa_state_identifier_t>::identifier_t;
    };

    struct nfa_state  {
        nfa_state_identifier_t id_;
        bool accepting_ = false;
        symbol_identifier_t accepted_symbol_;


        using input_symbol = std::variant<std::monostate, char>;
        std::multimap<input_symbol, nfa_state_identifier_t> transitions_;

        nfa_state(nfa_state_identifier_t id) : id_(id) {};

        void add_transition(char c, nfa_state_identifier_t to_state_id) {
            transitions_.emplace(c, to_state_id);
        }

        void add_epsilon_transition(nfa_state_identifier_t to_state_id) {
            transitions_.emplace(std::monostate{}, to_state_id);
        }
    };

    struct nfa_machine {
        std::map<nfa_state_identifier_t, nfa_state> states_;
        nfa_state_identifier_t start_state_;
        std::set<nfa_state_identifier_t> accepting_states_;
        // If partial is `true` then the
        // accepting states are simply final states for
        // this machine. They don't actually "accept" any
        // token.
        // This signals to the combining nethods that
        // if there are any transitions out of these states
        // after combining, then they should no longer be 
        // accepting.
        bool partial_ = false;

        nfa_machine(bool p) : partial_(p) {}
        nfa_machine() = default;

        // build a simple machine that recognizes the char c
        nfa_machine(char c, bool partial = true);

        nfa_machine & add_transition(nfa_state_identifier_t from_state_id, 
                nfa_state_identifier_t to_state_id, char c);
        // yes, could have overloaded, but this seems clearer
        nfa_machine & add_epsilon_transition(nfa_state_identifier_t from_state_id, 
                nfa_state_identifier_t to_state_id);

        // These modify the base machine
        // These will work even if the same machine is passed in.
        // If the base machine is partial, then epsilons will be
        // added as necesary to have one accepting state in the
        // resulting machine
        nfa_machine &union_in(const nfa_machine &o);
        nfa_machine &concat_in(const nfa_machine &o);
        nfa_machine &close_in();

        // slightly faster than me.concat_in(nfa_machine{c})
        nfa_machine &concat_char(char c);

        static std::unique_ptr<nfa_machine> build_from_string(std::string_view input,
                symbol_identifier_t sym_id);

      private:
        nfa_state &append_epsilon_state();

        nfa_state & copy_in(const nfa_machine &o);

    };

    /*=================================================================*/
    struct dfa_state_identifier_t : util::identifier_t<dfa_state_identifier_t> {
        using util::identifier_t<dfa_state_identifier_t>::identifier_t;
    };

    struct dfa_state  {
        dfa_state_identifier_t id_;
        bool accepting_ = false;
        // the dfa may accept multiple symbols in the same
        // state. The decision as to which gets returned is
        // going to depend on which were passed as valid
        // (context aware lexing), length of the accepted string
        // relative position in the original grammar (early wins).
        std::set<symbol_identifier_t> accepted_symbol_;

        using input_symbol = char;
        std::map<input_symbol, dfa_state_identifier_t> transitions_;
    };

    struct dfa_machine {
        std::map<dfa_state_identifier_t, dfa_state> states_;
        dfa_state_identifier_t start_state_;
        std::set<dfa_state_identifier_t> accepting_states_;

    };
} // yalr
