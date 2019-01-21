#define CATCH_CONFIG_MAIN
#include "analyzer.hpp"
#include "parser.hpp"

#include <catch2/catch.hpp>

auto parse_string(const std::string& s) {
    auto f = s.begin();
    const auto e = s.end();

    std::cerr << "In parse_string\n";
    auto parse_tree = yalr::do_parse(f,e);
    return yalr::analyzer::analyze(parse_tree);
}

TEST_CASE("grammar", "[analyzer]") {
    // Sanity check - this is the minimal
    // spec acceptable to the analyzer
    auto tree = parse_string("term foo; goal rule X { => foo ; }");
    REQUIRE(bool(tree));

    std::cerr << "past sanity check\n";

    SECTION("analyze single quotes in productions", "[analyzer]") {
        auto tree = parse_string("term BAR 'bar' ; term FOO 'foo' ; goal rule A { => BAR ; => 'foo' ; }");
        REQUIRE(bool(tree));
    }

}

