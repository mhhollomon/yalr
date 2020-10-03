#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "parser.hpp"


auto parse_string(std::string s) { 

    auto ts = std::make_shared<yalr::text_source>("t11-grammar", std::move(s));
    auto p = yalr::yalr_parser(ts);
    // break this out to make it easier to debug
    auto tree = p.parse();
    return tree;
}

void output_errors(yalr::parse_tree const &t) {
    t.errors.output(std::cout);
}
TEST_CASE("termset - [parser]") {
    SUBCASE("just assoc") {
        auto tree = parse_string("termset foo @assoc=left  'X' x;");
        CHECK(tree.success);
    }
    SUBCASE("just prec") {
        auto tree = parse_string("termset <int> foo @prec=100 'x' abc;");
        CHECK(tree.success);
    }
    SUBCASE("prec first") {
        auto tree = parse_string("termset <@lexeme> arg @prec=foo @assoc=left 'X';");
        CHECK(tree.success);
    }
    SUBCASE("assoc first") {
        auto tree = parse_string("termset flah @assoc=left @prec='+' zed;");
        CHECK(tree.success);
    }
}
