#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "analyzer.hpp"
#include "parser.hpp"


using parser = yalr::yalr_parser;

auto parse_string(const std::string &s) {
    auto p = parser(std::make_shared<yalr::text_source>("test", std::string{s}));
    auto tree = p.parse();
    auto retval = yalr::analyzer::analyze(tree);
    for (auto const& e : retval->errors) {
        e.output(std::cout);
    }

    return retval;
}

TEST_CASE("grammar - [analyzer]") {
    // Sanity check - this is the minimal
    // spec acceptable to the analyzer
    auto tree = parse_string("term foo 'x'; goal rule X { => foo ; }");
    REQUIRE(bool(*tree));

    std::cerr << "past sanity check\n";

    SUBCASE("[analyzer] analyze single quotes in productions") {
        auto tree = parse_string("term BAR 'bar' ; term FOO 'foo' ; goal rule A { => BAR ; => 'foo' ; }");
        CHECK(bool(*tree));
    }

}

TEST_CASE("[analyzer] Error on dups") {
    SUBCASE("[analyzer] dup skip/term") {
        auto tree = parse_string("term foo 'x';skip foo 'y'; goal rule X { => foo ; }");
        std::cerr << "Tree success == " << tree->success << "\n";
        CHECK_FALSE(bool(*tree));
    }

    SUBCASE("analyzer] dup skip/term on pattern") {
        auto tree = parse_string("term foo 'x';skip bar 'x'; goal rule X { => foo ; }");
        CHECK_FALSE(bool(*tree));
    }
    
    SUBCASE("analyzer] dup skip/rule") {
        auto tree = parse_string("term foo 'x';skip bar 'y'; goal rule bar { => foo ; }");
        CHECK_FALSE(bool(*tree));
    }

    SUBCASE("analyzer] dup term/rule") {
        auto tree = parse_string("term foo 'x';skip bar 'y'; goal rule foo { => 'x' ; }");
        CHECK_FALSE(bool(*tree));
    }
}

TEST_CASE("[analyzer] Can't use skips in rules") {
    SUBCASE("analyzer] skip by name") {
        auto tree = parse_string("term foo 'x';skip bar 'y'; goal rule A { => bar ; }");
        CHECK_FALSE(bool(*tree));
    }

    SUBCASE("analyzer] skip by pattern") {
        auto tree = parse_string("term foo 'x';skip bar 'y'; goal rule A { => 'y' ; }");
        CHECK_FALSE(bool(*tree));
    }
}

TEST_CASE("[analyzer] allow single quotes to define a new term") {
    SUBCASE("[analyzer] simple") {
        auto tree = parse_string("goal rule A { => 'bar' ; }");
        REQUIRE(*tree);
        auto sym = tree->symbols.find("'bar'");
        REQUIRE(sym);
        CHECK(sym->token_name() == "0TERM1");
    }
}

TEST_CASE("[analyzer] precedence") {
    auto tree = parse_string("precedence 100 'bar' ; goal rule A { => 'bar' 'baz' ; }");
    REQUIRE(*tree);
    auto sym = tree->symbols.find("'bar'");
    REQUIRE(sym);
    CHECK(sym->token_name() == "0TERM1");
    auto data = sym->get_data<yalr::symbol_type::terminal>();
    REQUIRE(data);
    CHECK(*(data->precedence) == 100);

    auto baz_sym = tree->symbols.find("'baz'");
    REQUIRE(baz_sym);
    CHECK(baz_sym->token_name() == "0TERM2");
    auto baz_data = baz_sym->get_data<yalr::symbol_type::terminal>();
    REQUIRE(baz_data);

    {
    CAPTURE(baz_data->pattern);
    CHECK(baz_data->pattern == "baz");
    }

    {
    CAPTURE(*(baz_data->precedence));
    CHECK(not baz_data->precedence);
    }

    // The precedence of a production is the precedence ofthe 
    // right-most terminal (unless otherwise specified). In this case,
    // the right-most terminal is inlne and has no precedence. So the 
    // production should alo not have precedence.
    //
    CAPTURE(*(tree->productions[0].precedence));
    CHECK(not tree->productions[0].precedence);
}
