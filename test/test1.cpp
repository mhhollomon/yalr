#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "parser.hpp"
#include <catch2/catch.hpp>


auto parse_string = [](const std::string& s) { 
    auto f = s.begin();
    const auto e = s.end();

    return yalr::do_parse(f,e);
};

TEST_CASE("Terminal parsing pattern", "[parser]") {

    auto tree = parse_string("term foo \"bar%^$\\\" ;");

    REQUIRE(tree.success);
    REQUIRE(tree.defs.size() == 1);

    auto term_def = std::get<yalr::ast::terminal>(tree.defs[0]);

    REQUIRE(term_def.name == "foo");
    REQUIRE(term_def.pattern == "\"bar%^$\\\"");
}

TEST_CASE("Terminal parsing no pattern", "[parser]") {

    auto tree = parse_string("term foo;");

    REQUIRE(tree.success);
    REQUIRE(tree.defs.size() == 1);

    auto term_def = std::get<yalr::ast::terminal>(tree.defs[0]);

    REQUIRE(term_def.name == "foo");
    REQUIRE(term_def.pattern.empty());
}

TEST_CASE("Terminal parsing bad pattern", "[parser]") {

    auto tree = parse_string("term foo \"xxx;");
    REQUIRE(not tree.success);
}
