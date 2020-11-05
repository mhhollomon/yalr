#pragma once

#include "utils.hpp"
#include "symbols.hpp"
#include <map>
#include <variant>

namespace yalr::codegen {

    struct nfa_state_identifier_t : util::identifier_t<nfa_state_identifier_t> {
        using util::identifier_t<nfa_state_identifier_t>::identifier_t;
    };

    enum symbol_type_t : char {
        sym_undef = 0,
        sym_char, // char is a range - for single letter first = last
        sym_class,
        sym_epsilon = 0x7f
    };

    struct input_symbol_t {
        symbol_type_t sym_type_ = sym_undef;
        char first_ = 0;
        char last_ = 0;

        bool operator <(const input_symbol_t &lhs) const {
            return ((sym_type_ < lhs.sym_type_) or 
                (sym_type_ == lhs.sym_type_ and first_ < lhs.first_) or
                (sym_type_ == lhs.sym_type_ and first_ == lhs.first_ and 
                 last_ < lhs.last_));
        }

        // grumble. Because I want the single parmater constructor,
        // this struct is no longer an "aggregate". So I have to explicitly
        // provide the full constructor.
        //
        input_symbol_t(symbol_type_t s, char f, char l) :
            sym_type_(s), first_(f), last_(l) {}

        input_symbol_t(char c) : sym_type_(sym_char), first_(c), last_(c) {}
        input_symbol_t() = default;
    };

    extern input_symbol_t epsilon_symbol;

    struct nfa_state  {
        nfa_state_identifier_t id_;
        bool accepting_ = false;
        symbol_identifier_t accepted_symbol_;


        std::multimap<input_symbol_t, nfa_state_identifier_t> transitions_;

        nfa_state(nfa_state_identifier_t id) : id_(id) {};

        void add_transition(input_symbol_t symb, nfa_state_identifier_t to_state_id) {
            transitions_.emplace(symb, to_state_id);
        }

        void add_transition(char c, nfa_state_identifier_t to_state_id) {
            transitions_.emplace(input_symbol_t{c}, to_state_id);
        }

        void add_epsilon_transition(nfa_state_identifier_t to_state_id) {
            transitions_.emplace(epsilon_symbol, to_state_id);
        }
    };

    struct nfa_machine {
        std::map<nfa_state_identifier_t, nfa_state> states_;
        nfa_state_identifier_t start_state_id_;
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

        nfa_machine(input_symbol_t sym);

        // create a simple partial two state machine
        // that accepts c.
        nfa_machine(char c) : nfa_machine(input_symbol_t{c}) {}

        nfa_machine() = default;

        // add a new state with a new id and return a reference to it.
        //
        nfa_state & add_state();

        // Transition helpers.
        // These will throw if the from_state_id is invalid.
        // These will silently work even if the to_stat_id is invalid
        //
        nfa_machine & add_transition(input_symbol_t symb, nfa_state_identifier_t from_state_id, 
                nfa_state_identifier_t to_state_id);

        nfa_machine & add_transition(char c, nfa_state_identifier_t from_state_id, 
                nfa_state_identifier_t to_state_id);

        nfa_machine & add_epsilon_transition(nfa_state_identifier_t from_state_id, 
                nfa_state_identifier_t to_state_id);

        // These modify the base machine
        // These will work even if the same machine is passed in.
        //
        nfa_machine &union_in(const nfa_machine &o);
        nfa_machine &concat_in(const nfa_machine &o);
        nfa_machine &close_in();
        nfa_machine &plus_in();
        nfa_machine &option_in();

        // really should be private
        nfa_state & copy_in(const nfa_machine &o);

        // slightly faster than me.concat_in(nfa_machine{c})
        // and creates fewer epsilons
        nfa_machine &concat_char(char c);

        static std::unique_ptr<nfa_machine> build_from_string(std::string_view input,
                symbol_identifier_t sym_id);

        struct run_results {
            bool matched;
            int length;
        };
       
        run_results run(std::string_view input);
        bool health_check(std::ostream &strm);
        void dump(std::ostream &strm);

       friend struct nfa_builder;


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

        std::map<input_symbol_t, dfa_state_identifier_t> transitions_;
    };

    struct dfa_machine {
        std::map<dfa_state_identifier_t, dfa_state> states_;
        dfa_state_identifier_t start_state_;
        std::set<dfa_state_identifier_t> accepting_states_;

    };
} // yalr
