#define CATCH_CONFIG_MAIN
#include "parser.hpp"
#include <catch2/catch.hpp>


auto parse_string = [](const std::string& s) { 
    auto f = s.begin();
    const auto e = s.end();

    return yalr::do_parse(f,e);
};

TEST_CASE("Quoted Pattern", "[parser]") {
    // Sanity check - this is the minimal
    // spec acceptable to the parser. (the analyzer requires more).
    auto tree = parse_string("term foo;");
    REQUIRE(tree.success);

    SECTION("Simple Pattern") {
        auto tree = parse_string("term foo \"b/*ar%^$\\\" ;");

        REQUIRE(tree.success);
        REQUIRE(tree.defs.size() == 1);

        auto term_def = std::get<yalr::ast::terminal>(tree.defs[0]);

        REQUIRE(term_def.name == "foo");
        REQUIRE(term_def.pattern == "\"b/*ar%^$\\\"");
    }

    SECTION("Bad Pattern (not terminated)") {

        auto tree = parse_string("term foo \"xxx;");
        REQUIRE(not tree.success);
    }
}

