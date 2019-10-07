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

parser mk_parser(const std::string &s) {
    return parser(std::make_shared<yalr::text_source>("test", std::string{s}));
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

        std::stringstream ss;

        p.stream_errors(ss);
        std::string expected = R"x(test:1:3: error:Unterminated comment starting here
  //     X
~~~^
)x";
        CHECK(ss.str() == expected);
   

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

    SUBCASE("negative") {
        auto p = mk_parser("rule");
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
    SUBCASE("positive 1") {
        auto p = mk_parser("'xyz\\' \"'");
        auto lexeme = p.match_singlequote();
        CHECK(lexeme);
        CHECK(lexeme->text == "'xyz\\' \"'");
        CHECK(p.eoi());
        CHECK(p.current_loc.offset == 9);
    }
    SUBCASE("positive 2") {
        auto p = mk_parser("'xyz\\\nabc' cccc");
        auto lexeme = p.match_singlequote();
        CHECK(lexeme);
        CHECK(lexeme->text == "'xyz\\\nabc'");
        CHECK(not p.eoi());
        CHECK(p.current_loc.offset == 10);
    }
    SUBCASE("negative - [parser]") {
        {
        auto p = mk_parser("'abc\nxyz'");
        auto lexeme = p.match_singlequote();
        CHECK_FALSE(lexeme);
        CHECK(p.error_list.size() == 1);
        }
        {
        auto p = mk_parser("'xyz");
        auto lexeme = p.match_singlequote();
        CHECK_FALSE(lexeme);
        CHECK(p.error_list.size() == 1);
        }

    }
}

TEST_CASE("match_regex() - [parser]") {
    SUBCASE("positive 1") {
        auto p = mk_parser("r:abc[\"\\ ]: ");
        auto lexeme = p.match_regex();
        CHECK(lexeme);
        CHECK(lexeme->text == "r:abc[\"\\ ]:");
        CHECK(not p.eoi());
        CHECK(p.current_loc.offset == 11);
    }
    SUBCASE("positive 2") {
        auto p = mk_parser(R"d(r:\s+ )d");
        auto lexeme = p.match_regex();
        CHECK(lexeme);
        CHECK(lexeme->text == "r:\\s+");
        CHECK(not p.eoi());
        CHECK(p.current_loc.offset == 5);
    }
    SUBCASE("negative") {
        auto p = mk_parser("x:abc[\"\\ ]: ");
        auto lexeme = p.match_regex();
        CHECK(not lexeme);
        CHECK(p.current_loc.offset == 0);
    }
}
