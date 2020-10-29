#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#define DOCTEST_CONFIG_NO_EXCEPTIONS_BUT_WITH_ALL_ASSERTS
#include "doctest.h"

#include "codegen/fsm_builders.hpp"

using namespace yalr;
using namespace std::literals::string_view_literals;


TEST_CASE("[nfa_machine] - concat_char") {
    auto machine = codegen::nfa_machine{true};
    REQUIRE(machine.partial_ == true);

    machine.concat_char('a');
    CHECK(machine.states_.size() == 2);
    CHECK(machine.accepting_states_.size() == 1);

    machine.concat_char('b');
    CHECK(machine.states_.size() == 4);
    CHECK(machine.accepting_states_.size() == 1);
}

TEST_CASE("[nfa_builder] - simple string") {

    auto epsilon = std::monostate{};

    auto id = symbol_identifier_t::get_next_id();
    auto machine = codegen::nfa_machine::build_from_string("ab", id);

    CHECK(machine->states_.size() == 4);
    CHECK(machine->accepting_states_.size() == 1);

    // --- Start State
    auto state = machine->states_.at(machine->start_state_);
    CHECK(state.accepting_ == false);
    CHECK(state.transitions_.size() == 1);

    auto trans_iter = state.transitions_.find('a');
    CHECK(trans_iter != state.transitions_.end());
    CHECK(std::get<char>(trans_iter->first) == 'a');

    // -- next state the old 'a' aceepting state
    state = machine->states_.at(trans_iter->second);
    CHECK(state.accepting_ == false);
    CHECK(state.transitions_.size() == 1);

    trans_iter = state.transitions_.find(epsilon);
    CHECK(trans_iter != state.transitions_.end());
    CHECK(std::get<std::monostate>(trans_iter->first) == epsilon);

    // -- next state : the old b start state
    state = machine->states_.at(trans_iter->second);
    CHECK(state.accepting_ == false);
    CHECK(state.transitions_.size() == 1);

    trans_iter = state.transitions_.find('b');
    CHECK(trans_iter != state.transitions_.end());
    CHECK(std::get<char>(trans_iter->first) == 'b');

    // -- last state : the accepting state
    state = machine->states_.at(trans_iter->second);
    CHECK(state.accepting_ == true);
    CHECK(state.transitions_.size() == 0);
    CHECK(state.accepted_symbol_ == id);
}

TEST_CASE("[nfa_builder] - back slashes") {
    auto id = symbol_identifier_t::get_next_id();
    auto machine = codegen::nfa_machine::build_from_string("\t", id);

    CHECK(machine->states_.size() == 2);

    auto state = machine->states_.at(machine->start_state_);
    CHECK(state.accepting_ == false);
    CHECK(state.transitions_.size() == 1);
}

TEST_CASE("[nfa_builder] - simple char class") {
    auto id = symbol_identifier_t::get_next_id();
    auto machine = codegen::nfa_machine::build_from_string("[bc]", id);

    CHECK(machine->states_.size() == 2);

    auto state = machine->states_.at(machine->start_state_);
    CHECK(state.accepting_ == false);
    CHECK(state.transitions_.size() == 2);
}


TEST_CASE("[nfa_builder] - union_in") {
    auto id = symbol_identifier_t::get_next_id();
    auto me = codegen::nfa_machine::build_from_string("a", id);
    auto you = codegen::nfa_machine::build_from_string("c", id);

    auto old_start = me->start_state_;
    me->union_in(*you);

    CHECK(me->start_state_ == old_start);
    CHECK(me->states_.size() == 4);
    CHECK(me->accepting_states_.size() == 2);
    auto trans_iter = me->states_.at(me->start_state_).transitions_.find(std::monostate{});
    CHECK(trans_iter != me->states_.at(me->start_state_).transitions_.end());
}

TEST_CASE("[nfa_builder] - build_nfa") {
    symbol_table sym_tab;

    terminal_symbol t;
    t.name = "FOO"sv;
    t.type_str = "void"sv;
    t.pattern = "foo"sv;
    t.token_name = "FOO"sv;
    t.pat_type = pattern_type::string;
    sym_tab.add("FOO"sv, t);

    t.name = "BAR"sv;
    t.pattern = "bar"sv;
    t.token_name = "BAR"sv;
    sym_tab.add("BAR"sv, t);

    auto retval = codegen::build_nfa(sym_tab);

    CHECK(retval->states_.size() == 13);
    CHECK(retval->accepting_states_.size() == 2);
    CHECK(retval->partial_ == false);

    auto & trans = retval->states_.at(retval->start_state_).transitions_;
    CHECK(trans.size() == 2);
    auto trans_iter = trans.find(std::monostate{});
    CHECK(trans_iter != trans.end());

    INFO( "epsilon state = " << trans_iter->second << "\n");
    auto &state = retval->states_.at(trans_iter->second);
    CHECK(state.accepting_ == false);
}

TEST_CASE("[nfa_build] - plus_in" ) {
    auto id = symbol_identifier_t::get_next_id();
    auto me = codegen::nfa_machine::build_from_string("a+", id);

    CHECK(me->states_.size() == 2 );
    CHECK(me->accepting_states_.size() == 1);
    auto final_state_id = *(me->accepting_states_.begin());
    CHECK(me->states_.at(final_state_id).transitions_.size() == 1);
    CHECK(final_state_id != me->start_state_);
}

TEST_CASE("[nfa_machine] - runner") {
    auto id = symbol_identifier_t::get_next_id();
    auto nfa = codegen::nfa_machine::build_from_string("a+", id);

    auto res = nfa->run("aaa");
    CHECK(res.matched == true);
    CHECK(res.length == 3);

    res = nfa->run("");
    CHECK(res.matched == false);

    res = nfa->run("aab");
    CHECK(res.matched == true);
    CHECK(res.length == 2);

}

TEST_CASE("[nfa_machine] - runner") {
    auto id = symbol_identifier_t::get_next_id();
    auto nfa = codegen::nfa_machine::build_from_string("a+b+", id);
    auto res = nfa->run("aaa");
    CHECK(res.matched == false);

    res = nfa->run("aaab");
    CHECK(res.matched == true);
    CHECK(res.length == 4);

    res = nfa->run("bb");
    CHECK(res.matched == false);
} 

TEST_CASE("[nfa_machine] - runner") {
    auto id = symbol_identifier_t::get_next_id();
    auto nfa = codegen::nfa_machine::build_from_string("b?a", id);

    auto res = nfa->run("ba");
    CHECK(res.matched == true);
    CHECK(res.length == 2);

    res = nfa->run("b");
    CHECK(res.matched == false);

    res = nfa->run("a");
    CHECK(res.matched == true);
    CHECK(res.length == 1);

}
