#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "parser.hpp"
#include <catch2/catch.hpp>


auto parse_string = [](const std::string& s) { 
    auto f = s.begin();
    const auto e = s.end();

    return yalr::do_parse(f,e);
};

TEST_CASE("Skipparser", "[parser]") {
    // Sanity check - this is the minimal
    // spec acceptable to the parser. (the analyzer requires more).
    auto tree = parse_string("term foo;");
    REQUIRE(tree.success);

    SECTION("new lines are handled") {
        auto tree = parse_string("term\n     foo\n\n  ;\n\n   ");
        REQUIRE(tree.success);
        REQUIRE(tree.defs.size() == 1);
    }

    SECTION("C style comments are handled") {
        auto tree = parse_string(R"(
/* starting comment */
term /* */
     foo /*  term bar; *** /*  dddd */

/*
 * Long, multi-line comment
 *
 * ******************/
; /* // ****/)"
                );
        REQUIRE(tree.success);
        REQUIRE(tree.defs.size() == 1);
    }

    SECTION("C++ style comments are handled") {
        auto tree = parse_string(R"(
// Here is a comment
term /* // */ foo // /* */
; //
//   xxxxxx 
//   )"
                );
        REQUIRE(tree.success);
        REQUIRE(tree.defs.size() == 1);
    }

}
