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


TEST_CASE("[nfa_machine] - a+") {
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

TEST_CASE("[nfa_machine] - a+b+") {
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

TEST_CASE("[nfa_machine] - optional") {
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

TEST_CASE("[nfa_machine] - parens") {
    auto id = symbol_identifier_t::get_next_id();
    auto nfa = codegen::nfa_machine::build_from_string("(ab)+", id);

    auto res = nfa->run("ab");
    CHECK(res.matched == true);
    CHECK(res.length == 2);

    res = nfa->run("abab");
    CHECK(res.matched == true);
    CHECK(res.length == 4);

    nfa = codegen::nfa_machine::build_from_string("c(a(bc)?d)+e", id);

    res = nfa->run("cadabcde");
    CHECK(res.matched == true);
    CHECK(res.length == 8);
}

TEST_CASE("[nfa_machine] - simple union") {
    auto id = symbol_identifier_t::get_next_id();
    auto nfa = codegen::nfa_machine::build_from_string("g|h", id);

    auto res = nfa->run("g");
    CHECK(res.matched == true);
    CHECK(res.length == 1);

    res = nfa->run("h");
    CHECK(res.matched == true);
    CHECK(res.length == 1);

    nfa = codegen::nfa_machine::build_from_string("ag|h+", id);

    res = nfa->run("hhh");
    CHECK(res.matched == true);
    CHECK(res.length == 3);

    res = nfa->run("agh");
    CHECK(res.matched == true);
    CHECK(res.length == 2);

    nfa = codegen::nfa_machine::build_from_string("a(g|h)+", id);
    CHECK(nfa->health_check(std::cout));
    //nfa->dump(std::cout);

    res = nfa->run("agh");
    CHECK(res.matched == true);
    CHECK(res.length == 3);
}


/*------------------------------------------------------*/

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

    auto res = retval->run("foo");
    CHECK(res.matched == true);
    CHECK(res.length == 3);

    res = retval->run("bar");
    CHECK(res.matched == true);
    CHECK(res.length == 3);

}
