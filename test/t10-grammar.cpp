#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "parser.hpp"

#include <catch2/catch.hpp>

auto parse_string(const std::string& s) {
    auto f = s.begin();
    const auto e = s.end();

    std::cerr << "In parse_string\n";
    return yalr::do_parse(f,e);
}

TEST_CASE("grammar", "[parser]") {
    // Sanity check - this is the minimal
    // spec acceptable to the parser. (the analyzer requires more).
    auto tree = parse_string("term foo;");
    REQUIRE(tree.success);

    std::cerr << "past sanity check\n";

    SECTION("Parse single quotes in productions", "[parser]") {
        auto tree = parse_string("term FOO 'foo' ; rule A { => FOO ; => 'foo' ; }");
        REQUIRE(tree.success);
    }

}

