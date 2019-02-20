#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "parser.hpp"

namespace ast = yalr::ast;
using parser = yalr::parser::yalr_parser;

#include <catch2/catch.hpp>

ast::grammar parse_string(std::string_view sv) { 
    auto p = parser(sv);
    return p.parse(); 
}

TEST_CASE("grammar", "[parser]") {

    SECTION("Parse single quotes in productions", "[parser]") {
        auto tree = parse_string("term FOO 'foo' ; rule A { => FOO ; => 'foo' ; }");
        REQUIRE(tree.success);
    }

    SECTION("type names for terms", "[parser]") {
        auto tree = parse_string("term<int> FOO 'foo' ; rule A { => FOO ; => 'foo' ; }");
        REQUIRE(tree.success);
        auto term = std::get<ast::terminal>(tree.defs[0]);
        REQUIRE(term.type_str == "int");

        tree = parse_string("term<std::string> FOO 'foo' ; rule A { => FOO ; => 'foo' ; }");
        REQUIRE(tree.success);
        term = std::get<ast::terminal>(tree.defs[0]);
        REQUIRE(term.type_str == "std::string");

        tree = parse_string("term<unsigned long long> FOO 'foo' ; rule A { => FOO ; => 'foo' ; }");
        REQUIRE(tree.success);
        term = std::get<ast::terminal>(tree.defs[0]);
        REQUIRE(term.type_str == "unsigned long long");
        
    }

    SECTION("action block in terms", "[parser]") {
        auto tree = parse_string(
R"DELIM(term<int> FOO 'foo' <%{ // here is my action block
    /* don't need a semicolon */
    return 3;
    }%>
term <std::string> BAR 'bar' ;
)DELIM" );
        REQUIRE(tree.success);
        // BAR should be in the list
        REQUIRE(tree.defs.size() == 2);
    }

    SECTION("Type names for rules", "[parser]") {
        auto tree = parse_string("rule <int> A { => A A ; => ; }");
        REQUIRE(tree.success);
        auto term = std::get<ast::rule_def>(tree.defs[0]);
        REQUIRE(term.type_str == "int");

        tree = parse_string("rule <my_thing<int>> A { => A A ; => ; }");
        REQUIRE(tree.success);
        term = std::get<ast::rule_def>(tree.defs[0]);
        REQUIRE(term.type_str == "my_thing<int>");
    }

}

