#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "parser.hpp"


auto parse_string(std::string s) { 

    auto ts = std::make_shared<yalr::text_source>("t10-grammar", std::move(s));
    auto p = yalr::yalr_parser(ts);
    return p.parse(); 
}

TEST_CASE("grammar - [parser]") {

    SUBCASE("Parse single quotes in productions - [parser]") {
        auto tree = parse_string("term FOO 'foo' ; rule A { => FOO ; => 'foo' ; }");
        CHECK(tree.success);
    }

    SUBCASE("type names for terms - [parser]") {
        auto tree = parse_string("term<int> FOO 'foo' ; rule A { => FOO ; => 'foo' ; }");
        CHECK(tree.success);
        auto term = std::get<yalr::terminal>(tree.statements[0]);
        CHECK(term.type_str->text == "int");

        tree = parse_string("term<std::string> FOO 'foo' ; rule A { => FOO ; => 'foo' ; }");
        CHECK(tree.success);
        term = std::get<yalr::terminal>(tree.statements[0]);
        CHECK(term.type_str->text == "std::string");

        tree = parse_string("term<unsigned long long> FOO 'foo' ; rule A { => FOO ; => 'foo' ; }");
        CHECK(tree.success);
        term = std::get<yalr::terminal>(tree.statements[0]);
        CHECK(term.type_str->text == "unsigned long long");
        
    }

    SUBCASE("action block in terms - [parser]") {
        auto tree = parse_string(
R"DELIM(term<int> FOO 'foo' <%{ // here is my action block
    /* don't need a semicolon */
    return 3;
    }%>
term <std::string> BAR 'bar' ;
)DELIM" );
        CHECK(tree.success);
        // BAR should be in the list
        CHECK(tree.statements.size() == 2);
    }

    SUBCASE("Type names for rules - [parser]") {
        auto tree = parse_string("rule <int> A { => A A ; => ; }");
        CHECK(tree.success);
        auto rule = std::get<yalr::rule>(tree.statements[0]);
        CHECK(rule.type_str->text == "int");

        tree = parse_string("rule <my_thing<int>> A { => A A ; => ; }");
        CHECK(tree.success);
        rule = std::get<yalr::rule>(tree.statements[0]);
        CHECK(rule.type_str->text == "my_thing<int>");

        tree = parse_string("rule <my_thing::your_thing<funky::int>> A { => A A ; => ; }");
        CHECK(tree.success);
        rule = std::get<yalr::rule>(tree.statements[0]);
        CHECK(rule.type_str->text == "my_thing::your_thing<funky::int>");
    }

    SUBCASE("aliases for terms - [parser]") {
        auto tree = parse_string("rule A { => i:FOO ; => j:'foo' ; }");
        CHECK(tree.success);
        auto ruleA = std::get<yalr::rule>(tree.statements[0]);
        auto alt = ruleA.alternatives[0];
        CHECK(alt.items[0].alias->text == "i");
        alt = ruleA.alternatives[1];
        CHECK(alt.items[0].alias->text == "j");
    }

}

