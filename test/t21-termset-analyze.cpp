#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "analyzer.hpp"
#include "parser.hpp"


using parser = yalr::yalr_parser;

auto parse_string(const std::string &s) {
    auto p = parser(std::make_shared<yalr::text_source>("test", std::string{s}));
    auto tree = p.parse();
    REQUIRE(tree.success);
    auto retval = yalr::analyzer::analyze(tree);
    retval->errors.output(std::cout);
    //yalr::analyzer::pretty_print(*retval, std::cout);

    return retval;
}

TEST_CASE("termset simple - [analyzer]") {
    auto tree = parse_string("termset FOO 'x' ; goal rule A { => FOO; }");
    REQUIRE(bool(*tree));

    SUBCASE("[analyzer] check term is created") {
        auto xterm = tree->symbols.find("'x'");
        REQUIRE(xterm);
        auto term_data = xterm->get_data<yalr::symbol_type::terminal>();
        REQUIRE(term_data);
        CHECK(term_data->type_str == "void");
    }

    SUBCASE("[analyzer] check rule is created") {
        auto rule = tree->symbols.find("FOO");
        REQUIRE(rule);
        auto rule_data = rule->get_data<yalr::symbol_type::rule>();
        REQUIRE(rule_data);
        CHECK(rule_data->type_str == "void");
        // There should 1 production for the goal rule
        // 1 production for the synthetic goal and
        // 1 production for the termset.
        CHECK(tree->productions.size() == 3);
    }

}

TEST_CASE("termset name clashes - [analyzer]") {
    SUBCASE("[analyzer] - termset and rule can't have same name") {
        auto tree = parse_string("termset FOO 'x' ; goal rule FOO { => 'y'; }");
        CHECK_FALSE(bool(*tree));
        CHECK(tree->errors.errors.front().message == "'FOO' has already been defined as a rule");
    }
    SUBCASE("[analyzer] - termset and rule can't have same name") {
        auto tree = parse_string("goal rule FOO { => 'y'; } termset FOO 'x' ; ");
        CHECK_FALSE(bool(*tree));
        CHECK(tree->errors.errors.front().message == "'FOO' has already been defined as a rule");
    }

    SUBCASE("[analyzer] - can't use a rule name in a termset") {
        auto tree = parse_string("goal rule FOO { => 'y'; } termset BLAH FOO 'x' ; ");
        CHECK_FALSE(bool(*tree));
        CHECK(tree->errors.errors.front().message == "symbol reference is not a terminal");
    }

    SUBCASE("[analyzer] - can't define the terminal after the termset") {
        auto tree = parse_string("termset FOO 'x' ; term X 'x' ; goal rule A { => 'z' ; }");
        CHECK_FALSE(bool(*tree));
        CHECK(tree->errors.errors.front().message == "pattern ''x'' has already been defined by terminal 'x'");
    }
}

TEST_CASE("Termset typing - [analyzer]") {

    SUBCASE("[analyzer] - @lexeme is okay") {
        auto tree = parse_string("termset <@lexeme> TS 'x' 'y' ; goal rule A { => TS ; }");
        REQUIRE(bool(*tree));
        auto rule = tree->symbols.find("TS");
        REQUIRE(rule);
        auto rule_data = rule->get_data<yalr::symbol_type::rule>();
        REQUIRE(rule_data);
        CHECK(rule_data->type_str == "std::string");
        CHECK(tree->productions.front().action.find("_v1") != std::string_view::npos);
    }

    SUBCASE("[analyzer] - typed termsets need an action") {
        auto tree = parse_string("termset <int> TS 'x' ; goal rule A { => TS ; }");
        REQUIRE_FALSE(bool(*tree));
        CHECK(tree->errors.errors.front().message.find("must have an action") != std::string_view::npos);
    }

    SUBCASE("[analyzer] - predefined terms must match type") {
        auto tree = parse_string("term A 'a' ; termset <@lexeme> TS A ; goal rule B { => 'b' ; }");
        REQUIRE_FALSE(bool(*tree));
        CHECK(tree->errors.errors.front().message.find("does not match") != std::string_view::npos);
    }

    SUBCASE("[analyzer] - void termset allow any type terminal") {
        auto tree = parse_string("term <int> A 'a' <%{ return 1; }%> termset TS A ; goal rule B { => 'b' ; }");
        REQUIRE(bool(*tree));
    }

    SUBCASE("[analyzer] - action gets copied to terminals") {
        auto tree = parse_string("term <int> a 'a' <%{ return snarfle(); }%> termset <int> TS a 'b' <%{ return fleegle(); }%> goal rule B { => 'b' ; }");

        REQUIRE(bool(*tree));
        auto term = tree->symbols.find("'b'");
        REQUIRE(term);
        auto term_data = term->get_data<yalr::symbol_type::terminal>();
        REQUIRE(term_data);
        CHECK(term_data->action.find("fleegle") != std::string_view::npos);
        // make sure we didn't mess up the pre-defined term
        term = tree->symbols.find("'a'");
        REQUIRE(term);
        term_data = term->get_data<yalr::symbol_type::terminal>();
        REQUIRE(term_data);
        CHECK(term_data->action.find("fleegle") == std::string_view::npos);
        CHECK(term_data->action.find("snarfle") != std::string_view::npos);
    }

}

//
// TODO : Add test for @assoc and @prec handling
//

