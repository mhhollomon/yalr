/*
 * These test are to exercise the single_quote
 * parser.
 */
#define CATCH_CONFIG_MAIN

#include "type_name.hpp"
#include <catch2/catch.hpp>

#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/spirit/home/x3.hpp>

struct ast {
    std::string x;
    std::string q;
    std::string y;
};

BOOST_FUSION_ADAPT_STRUCT(ast, x, q, y);

namespace x3 = boost::spirit::x3;

const auto top_rule = x3::rule<struct rule_tag, ast>("top_rule") =
    x3::string("X") >> yalr::parser::type_name() >> x3::string("Y");


// return the <bool, int, ast>
template<typename Rule_T>
auto parse_string(const std::string& s, const Rule_T& the_rule) { 
    auto f = s.begin();
    const auto e = s.end();

    ast a;
    bool r = x3::phrase_parse(f, e, the_rule, x3::space, a);

    return std::make_tuple(r, e-f, a);
};

TEST_CASE("Type Name", "[parser]") {

    SECTION("Simple Pattern") {
        auto [r, dist, a] = parse_string("X <int> Y", top_rule);

        REQUIRE(r);
        REQUIRE(dist == 0);
        REQUIRE(a.x == "X"); REQUIRE(a.q == "int"); REQUIRE(a.y == "Y");

    }

    SECTION("Embedded <>") {
        auto [r, dist, a] = parse_string(
R"F(X<std::pair<x, y>> Y)F"
            , top_rule);
        REQUIRE(r);
        REQUIRE(dist == 0);
        REQUIRE(a.x == "X"); REQUIRE(a.q == R"F(std::pair<x, y>)F"); REQUIRE(a.y == "Y");
    }


    SECTION("Bad Pattern (not terminated)") {
        auto [r, dist, a] = parse_string("X  <long<> Y", top_rule);
        REQUIRE(not r);
        REQUIRE(dist > 0);
    }
}

