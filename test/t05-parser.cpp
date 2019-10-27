/*
 * These test are to exercise the parts of the parser class
 * These focus on the lower level "building blocks".
 * Larger functions should be tested as a part of the overall grammar
 * in t10 or other.
 */

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include <sstream>      // std::stringstream

#include "parser.cpp"

using parser = yalr::parser_guts;
using namespace std::string_literals;

parser mk_parser(const std::string &s) {
    return parser(std::make_shared<yalr::text_source>("test", std::string{s}));
}

std::string error_string(parser& p) {
    std::stringstream ss;
    p.stream_errors(ss);

    return ss.str();
}

TEST_CASE("skip() - [parser]") {

    SUBCASE("new lines are accounted in the location") {
        auto p = mk_parser("   \n  X   \n   ");

        CHECK(p.skip());
        CHECK(p.current_loc.sv[0] == 'X');
        CHECK(p.current_loc.offset == 6);
    }

    SUBCASE("C style comments are handled") {
        /* ---- Simple comment --- */
        auto p = mk_parser("  /*  \n   */ \n  X\n\n     ");
        CHECK(p.skip());
        CHECK(p.current_loc.sv[0] == 'X');
        CHECK(p.current_loc.offset == 16);


        /* ----- unterminated --- */
        p =  mk_parser("  /*     X");
        CHECK(not p.skip());
        CHECK(p.eoi());
        CHECK(p.error_list.size() > 0);

        /* --- Multiline ---- */
        /* also make sure embedded openers don't cause problems */
        p = mk_parser("  /*   /*  \n This \n has // \n\tstuff \n /* */\n  :   /*\n */");
        CHECK(p.skip());
        CHECK(p.current_loc.sv[0] == ':');
        CHECK(p.current_loc.offset == 45);

    }

    SUBCASE("C++ style comments are handled") {
        /* ---- Simple comment --- */
        auto p = mk_parser("  //  Stuff in the comment    \n  X\n\n     ");
        CHECK(p.skip());
        CHECK(p.current_loc.sv[0] == 'X');
        CHECK(p.current_loc.offset == 33);

        /* ----- unterminated --- */
        p =  mk_parser("  //     X");
        CHECK(not p.skip());
        CHECK(p.eoi());
        CHECK(p.error_list.size() > 0);

        std::string expected = R"x(test:1:3: error:Unterminated comment starting here
  //     X
~~^
)x";
        CHECK(error_string(p) == expected);
   

        /* C inside C++ */
        p = mk_parser("  // /* // \n */\n X ");
        CHECK(p.skip());
        CHECK(p.current_loc.sv[0] == '*');
        CHECK(p.current_loc.offset == 13);
    }
}

TEST_CASE("match_keyword() - [parser]") {

    SUBCASE("positive") {
         auto p =  mk_parser("keyword(");
         CHECK(p.match_keyword("keyword"));
    }

    SUBCASE("negative") {
        auto p = mk_parser("keyword_ ");
        CHECK(not p.match_keyword("keyword"));
    }
}

TEST_CASE("expect_keyword() - [parser]") {

    SUBCASE("positive") {
         auto p =  mk_parser("keyword(");
         CHECK(p.expect_keyword("keyword"));
    }

    SUBCASE("negative") {
        auto p = mk_parser("keyword_ ");
        CHECK(not p.expect_keyword("keyword"));
        CHECK(p.error_list.size() == 1);
    }
}

TEST_CASE("match_identifier() - [parser]") {

    SUBCASE("positive") {
        auto p =  mk_parser("__foo__bar123_ ");
        auto lexeme = p.match_identifier();
        CHECK(lexeme);
        CHECK(lexeme->text == "__foo__bar123_");

        {
        auto p = mk_parser("ruleabc");
        auto lexeme = p.match_identifier();
        CHECK(lexeme);
        CHECK(lexeme->text == "ruleabc");
        }
    }

    SUBCASE("negative 1") {
        auto p = mk_parser("rule");
        auto lexeme = p.match_identifier();
        CHECK(not lexeme);
    }
    SUBCASE("negative 2") {
        auto p = mk_parser("1234");
        auto lexeme = p.match_identifier();
        CHECK(not lexeme);
    }
}

TEST_CASE("match_char() - [parser]") {
    SUBCASE("positive") {
        auto p = mk_parser("xabcde");
        CHECK(p.match_char('x'));
        CHECK(p.current_loc.offset == 1);
        CHECK(p.match_char('a'));
    }

    SUBCASE("negative") {
        auto p = mk_parser("");
        CHECK(not p.match_char(':'));
        {
        auto p = mk_parser("z");
        CHECK(not p.match_char('Z'));
        }
    }
}

TEST_CASE("match_singlequote() - [parser]") {
    SUBCASE("escaped char") {
        auto p = mk_parser("'\\''");
        auto lexeme = p.match_singlequote();
        REQUIRE(lexeme);
        CHECK(lexeme->text == "'\\''");
        CHECK(p.eoi());
        CHECK(p.current_loc.offset == 4);
    }

    SUBCASE("positive 1") {
        auto p = mk_parser("'xyz\\' \"'");
        auto lexeme = p.match_singlequote();
        REQUIRE(lexeme);
        CHECK(lexeme->text == "'xyz\\' \"'");
        CHECK(p.eoi());
        CHECK(p.current_loc.offset == 9);
    }
    SUBCASE("positive 2") {
        auto p = mk_parser("'xyz\\\nabc' cccc");
        auto lexeme = p.match_singlequote();
        REQUIRE(lexeme);
        CHECK(lexeme->text == "'xyz\\\nabc'");
        CHECK(not p.eoi());
        CHECK(p.current_loc.offset == 10);
    }
    SUBCASE("negative - [parser]") {
        {
        auto p = mk_parser("'abc\nxyz'");
        auto lexeme = p.match_singlequote();
        CHECK(p.error_list.size() == 1);
        }
        {
        auto p = mk_parser("'xyz");
        auto lexeme = p.match_singlequote();
        CHECK(p.error_list.size() == 1);
        }

    }
}

TEST_CASE("match_regex() - [parser]") {
    SUBCASE("positive 1") {
        auto p = mk_parser(R"d(r:abc["\\\ ]: )d");
        auto lexeme = p.match_regex();
        REQUIRE(lexeme);
        CHECK(lexeme->text == R"d(r:abc["\\\ ]:)d");
        CHECK(not p.eoi());
        CHECK(p.current_loc.offset == 13);
    }
    SUBCASE("positive 2") {
        auto p = mk_parser(R"d(r:\s+ )d");
        auto lexeme = p.match_regex();
        CHECK(lexeme);
        CHECK(lexeme->text == "r:\\s+");
        CHECK(not p.eoi());
        CHECK(p.current_loc.offset == 5);
    }
    SUBCASE("positive ri 1") {
        auto p = mk_parser(R"d(rf:abc["\\\ ]: )d");
        auto lexeme = p.match_regex();
        REQUIRE(lexeme);
        CHECK(lexeme->text == R"d(rf:abc["\\\ ]:)d");
        CHECK(not p.eoi());
        CHECK(p.current_loc.offset == 14);
    }
    SUBCASE("negative") {
        auto p = mk_parser("x:abc[\"\\ ]: ");
        auto lexeme = p.match_regex();
        CHECK(not lexeme);
        CHECK(p.current_loc.offset == 0);
    }
}

TEST_CASE("match_int() - [parser]") {
    SUBCASE("positive 1") {
        auto p = mk_parser("120");
        auto lexeme = p.match_int();
        CHECK(lexeme);
        CHECK(lexeme->text == "120");
    }
    SUBCASE("positive 2") {
        auto p = mk_parser("-120.03");
        auto lexeme = p.match_int();
        CHECK(lexeme);
        CHECK(lexeme->text == "-120");
    }
    SUBCASE("negative") {
        auto p = mk_parser("-.03");
        auto lexeme = p.match_int();
        CHECK(not lexeme);
        CHECK(p.current_loc.offset == 0);
    }
}

TEST_CASE("match_assoc() - [parser]") {
    SUBCASE("positive 1") {
        auto p = mk_parser("@assoc=left");
        auto lexeme = p.match_assoc();
        CHECK(lexeme);
        CHECK(lexeme->text == "left");
    }
    SUBCASE("negative 1") {
        auto p = mk_parser("@assoc= left");
        auto lexeme = p.match_assoc();
        CHECK(not lexeme);
        auto expected = R"(test:1:8: error:missing or incorrect associativity specification
@assoc= left
~~~~~~~^
)"s;

        CHECK(error_string(p) == expected);
    }

    SUBCASE("negative 2") {
        auto p = mk_parser("@assoc=1234");
        auto lexeme = p.match_assoc();
        CHECK(not lexeme);
        auto expected = R"(test:1:8: error:missing or incorrect associativity specification
@assoc=1234
~~~~~~~^
)"s;

        CHECK(error_string(p) == expected);
    }
}

TEST_CASE("match_precedence() - [parser]") {
    SUBCASE("positive 1") {
        auto p = mk_parser("@prec=-123");
        auto lexeme = p.match_precedence();
        CHECK(lexeme);
        CHECK(lexeme->text == "-123");
    }
    SUBCASE("positive 2") {
        auto p = mk_parser("@prec=some_other_term");
        auto lexeme = p.match_precedence();
        CHECK(lexeme);
        CHECK(lexeme->text == "some_other_term");
    }
    SUBCASE("positive 3") {
        auto p = mk_parser("@prec='+'");
        auto lexeme = p.match_precedence();
        CHECK(lexeme);
        CHECK(lexeme->text == "'+'");
    }
    SUBCASE("negative 1") {
        auto p = mk_parser("@prec= foo");
        auto lexeme = p.match_precedence();
        CHECK(not lexeme);
        auto expected = R"(test:1:7: error:Missing or bad precedence specifier
@prec= foo
~~~~~~^
)"s;

        CHECK(error_string(p) == expected);
    }

}

TEST_CASE("match_dotted_identifier") {
    SUBCASE("positive") {
        auto p = mk_parser("foo.bar");
        auto lexeme = p.match_dotted_identifier();
        REQUIRE(lexeme);
        CHECK(lexeme->text == "foo.bar");
    }
    SUBCASE("negative 1 - no dot") {
        auto p = mk_parser("this_has_no_dot");
        auto lexeme = p.match_dotted_identifier();
        CHECK(not lexeme);
        auto expected = R"(test:1:16: error:Expecting a dotted identifier
this_has_no_dot
~~~~~~~~~~~~~~~^
)"s;
        CHECK(error_string(p) == expected);
    }
    SUBCASE("negative 1 - no dot") {
        auto p = mk_parser("this.end.in.a.dot.");
        auto lexeme = p.match_dotted_identifier();
        CHECK(not lexeme);
        auto expected = R"(test:1:19: error:Trailing dot on identifier
this.end.in.a.dot.
~~~~~~~~~~~~~~~~~~^
)"s;
        CHECK(error_string(p) == expected);
    }

}
