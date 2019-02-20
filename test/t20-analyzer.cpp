#define CATCH_CONFIG_MAIN
#include "analyzer.hpp"
#include "parser.hpp"

#include <catch2/catch.hpp>

namespace ast = yalr::ast;
using parser = yalr::parser::yalr_parser;

auto parse_string(std::string_view sv) {
    auto p = parser(sv);
    auto tree = p.parse();
    return yalr::analyzer::analyze(tree);
}

TEST_CASE("grammar", "[analyzer]") {
    // Sanity check - this is the minimal
    // spec acceptable to the analyzer
    auto tree = parse_string("term foo 'x'; goal rule X { => foo ; }");
    REQUIRE(bool(tree));

    std::cerr << "past sanity check\n";

    SECTION("analyze single quotes in productions", "[analyzer]") {
        auto tree = parse_string("term BAR 'bar' ; term FOO 'foo' ; goal rule A { => BAR ; => 'foo' ; }");
        REQUIRE(bool(tree));
    }

}

