#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "parser.hpp"


auto parse_string(std::string s) { 

    auto ts = std::make_shared<yalr::text_source>("t10-grammar", std::move(s));
    auto p = yalr::yalr_parser(ts);
    return p.parse(); 
}

void output_errors(yalr::parse_tree const &t) {
    for (auto const &e : t.errors) {
        e.output(std::cout);
    }
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
        auto ruleA = std::get<yalr::rule>(tree.statements[0]);
        auto alt = ruleA.alternatives[0];
        CHECK(alt.precedence->text == "2");
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

TEST_CASE("verbatim - [parser]") {
    SUBCASE("positive 1") {
        auto tree = parse_string("verbatim X <%{ ... }%>");
        for (auto const &e : tree.errors) {
            e.output(std::cout);
        }

        REQUIRE(tree.success);
        auto verb = std::get<yalr::verbatim>(tree.statements[0]);
        CHECK(verb.location.text == "X");
        CHECK(verb.text.text == " ... ");
    }

    SUBCASE("positive multipart location") {
        auto tree = parse_string("verbatim X.Y.Z <%{ ... }%>");
        REQUIRE(tree.success);
        auto verb = std::get<yalr::verbatim>(tree.statements[0]);
        CHECK(verb.location.text == "X.Y.Z");
        CHECK(verb.text.text == " ... ");
    }

    SUBCASE("negative - trailing dot") {
        auto tree = parse_string("verbatim X.Y. <%{ ... }%>");
        REQUIRE(not tree.success);
    }
}

TEST_CASE("associativity statement - [grammar]") {
    SUBCASE("positive") {
        auto tree = parse_string("associativity left 'a' X;");
        REQUIRE(tree.success);
        auto assoc = std::get<yalr::associativity>(tree.statements[0]);
        CHECK(assoc.assoc_text.text == "left");
        CHECK(assoc.symbol_refs.size() == 2);
    }
}

TEST_CASE("precedence statement - [grammar]") {
    SUBCASE("positive integer") {
        auto tree = parse_string("precedence 100 'a' X;");
        REQUIRE(tree.success);
        auto prec = std::get<yalr::precedence>(tree.statements[0]);
        CHECK(prec.prec_ref.text == "100");
        CHECK(prec.symbol_refs.size() == 2);
    }
    SUBCASE("positive identifier") {
        auto tree = parse_string("precedence FOO 'a' X;");
        REQUIRE(tree.success);
        auto prec = std::get<yalr::precedence>(tree.statements[0]);
        CHECK(prec.prec_ref.text == "FOO");
        CHECK(prec.symbol_refs.size() == 2);
    }
    SUBCASE("positive single quote") {
        auto tree = parse_string("precedence 'x' 'a' X;");
        output_errors(tree);
        REQUIRE(tree.success);
        auto prec = std::get<yalr::precedence>(tree.statements[0]);
        CHECK(prec.prec_ref.text == "'x'");
        CHECK(prec.symbol_refs.size() == 2);
    }
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
        auto tree = parse_string("termset <std::string> arg @prec=foo @assoc=left 'X';");
        CHECK(tree.success);
    }
    SUBCASE("assoc first") {
        auto tree = parse_string("termset flah @assoc=left @prec='+' zed;");
        CHECK(tree.success);
    }
}
