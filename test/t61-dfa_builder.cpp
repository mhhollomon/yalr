#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "codegen/fsm_builders.hpp"

using namespace yalr;
using namespace std::literals::string_view_literals;


TEST_CASE("[dfa_builder] - two simple strings") {
    auto id = symbol_identifier_t::get_next_id();
    auto me = codegen::nfa_machine::build_from_string("a", id);

    id = symbol_identifier_t::get_next_id();
    auto you = codegen::nfa_machine::build_from_string("c", id);

    me->union_in(*you);

    auto dfa = codegen::build_dfa(*me);

    CHECK(dfa->states_.size() == 3);
    CHECK(dfa->accepting_states_.size() == 2);

    auto state = dfa->states_.at(dfa->start_state_);
    CHECK(state.transitions_.size() == 2);
}

TEST_CASE("[dfa_builder] - two interlocking strings") {
    auto id = symbol_identifier_t::get_next_id();
    auto me = codegen::nfa_machine::build_from_string(":", id);

    id = symbol_identifier_t::get_next_id();
    auto you = codegen::nfa_machine::build_from_string(":=", id);

    me->union_in(*you);

    auto dfa = codegen::build_dfa(*me);

    CHECK(dfa->states_.size() == 3);
    CHECK(dfa->accepting_states_.size() == 2);

    auto state = dfa->states_.at(dfa->start_state_);
    CHECK(state.transitions_.size() == 1);
}

