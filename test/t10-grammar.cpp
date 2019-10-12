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

    SUBCASE("actions for alternatives - [parser]") {
        auto tree = parse_string("rule A { => <%{ return 1; }%> => x ; }");
        CHECK(tree.success);
        auto ruleA = std::get<yalr::rule>(tree.statements[0]);
        CHECK(ruleA.alternatives.size() == 2);
        auto alt = ruleA.alternatives[0];
        REQUIRE(alt.action);
        CHECK(alt.action->text == " return 1; ");
    }


}

TEST_CASE("namespace - [parser]") {
    SUBCASE("Identifier as the name") {
        auto tree = parse_string("namespace FOO ;");

        CHECK(tree.success);
        auto opt = std::get<yalr::option>(tree.statements[0]);
        CHECK(opt.setting.text == "FOO");
    }
    SUBCASE("quoted string as the name") {
        auto tree = parse_string("namespace 'one::two' ;");

        CHECK(tree.success);
        auto opt = std::get<yalr::option>(tree.statements[0]);
        CHECK(opt.setting.text == "one::two");
    }
}

TEST_CASE("assoc/prec in terms - [parser]") {
    SUBCASE("just assoc") {
        auto tree = parse_string("term X 'x' @assoc=left ;");
        CHECK(tree.success);
    }
    SUBCASE("just prec") {
        auto tree = parse_string("term X 'x' @prec=20;");
        CHECK(tree.success);
    }
    SUBCASE("prec first") {
        auto tree = parse_string("term X 'x' @prec=foo @assoc=left;");
        CHECK(tree.success);
    }
    SUBCASE("assoc first") {
        auto tree = parse_string("term X 'x' @assoc=left @prec='+';");
        CHECK(tree.success);
    }
    SUBCASE("duplicate") {
        auto tree = parse_string("term X 'x' @assoc=left @assoc=right ;\nterm Y 'y' @prec=123 @prec='x' ;");
        CHECK(not tree.success);
        CHECK(tree.errors.size() == 2);
    }
}

TEST_CASE("prec in rules - [parser]") {
    SUBCASE("positive") {
        auto tree = parse_string("rule A { => 'x' @prec=2; }");
        CHECK(tree.success);
    }
    SUBCASE("multiple") {
        auto tree = parse_string("rule A { => 'x' @prec=2 @prec=foo; }");
        CHECK(not tree);
        for (auto const &e : tree.errors) {
            e.output(std::cout);
        }
        CHECK(tree.errors.size() == 1);
    }
    SUBCASE("trailing item") {
        auto tree = parse_string("rule A { => 'x' @prec=2 'y'; }");
        CHECK(not tree);
        for (auto const &e : tree.errors) {
            e.output(std::cout);
        }
        CHECK(tree.errors.size() == 1);
    }
}


