/*
 * These test are to exercise the skipparser.
 */
#define CATCH_CONFIG_MAIN

#include "skipparser.hpp"
#include <catch2/catch.hpp>

#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/spirit/home/x3.hpp>

struct ast {
    std::string a;
    std::string b;
    std::string c;
};

BOOST_FUSION_ADAPT_STRUCT(ast, a, b, c);

namespace x3 = boost::spirit::x3;

const auto top_rule = x3::rule<struct rule_tag, ast>("top_rule") =
    x3::string("X") >> x3::string("Y") >> x3::string("Z");


// return the <bool, int, ast>
auto parse_string = [](const std::string& s) { 
    auto f = s.begin();
    const auto e = s.end();

    ast a;
    auto skipper = yalr::parser::skipparser();

    bool r = x3::phrase_parse(f, e, top_rule, skipper, a);

    return std::make_tuple(r, e-f, a);
};

TEST_CASE("Skipparser", "[parser]") {
    // Sanity check 
    auto [r, dist, a] = parse_string("XYZ");
    REQUIRE(r);
    REQUIRE(dist == 0);

    SECTION("new lines are handled") {
        auto [r, dist, a] = parse_string("\n  X\n\n\nY    Z\n\n\n  ");
        REQUIRE(r);
        REQUIRE(dist == 0);
        REQUIRE(a.a == "X"); REQUIRE(a.b == "Y"); REQUIRE(a.c == "Z");
    }

    SECTION("C style comments are handled") {
        auto [r, dist, a] = parse_string(R"(
/* starting comment */
X /* */
     Y /*  term bar; *** /*  dddd */

/*
 * Long, multi-line comment
 * X Y Z
 * ******************/
Z /* // ****/)"
                );
        REQUIRE(r);
        REQUIRE(dist == 0);
        REQUIRE(a.a == "X"); REQUIRE(a.b == "Y"); REQUIRE(a.c == "Z");
    }

    SECTION("C++ style comments are handled") {
        auto [r, dist, a] = parse_string(R"(
// Here is a comment
X /* // C++ embedded in C */ Y // C Z embedded in C++ /* */
//
//   xxxxxx 
//
Z  // finale   )"
                );
        REQUIRE(r);
        REQUIRE(dist == 0);
        REQUIRE(a.a == "X"); REQUIRE(a.b == "Y"); REQUIRE(a.c == "Z");
    }

}
