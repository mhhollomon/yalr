/*
 * These test are to exercise the parts of the parser class
 * These focus on the lower level "building blocks".
 * Larger functions should be tested as a part of the overall grammar
 * in t10 or other.
 */
#define CATCH_CONFIG_MAIN

#include "parser.hpp"
#include <catch2/catch.hpp>

using parser = yalr::parser::yalr_parser;

TEST_CASE("skip()", "[parser]") {

    SECTION("new lines are accounted in the location") {
        auto p = parser("   \n  X   \n   ");

        REQUIRE(p.skip());
        REQUIRE(p.current_loc.sv[0] == 'X');
        REQUIRE(p.current_loc.loc.offset == 6);
        REQUIRE(p.current_loc.loc.line == 2);
        REQUIRE(p.current_loc.loc.column == 3);
    }

    SECTION("C style comments are handled") {
        /* ---- Simple comment --- */
        auto p = parser("  /*     */ \n  X\n\n     ");
        REQUIRE(p.skip());
        REQUIRE(p.current_loc.sv[0] == 'X');
        REQUIRE(p.current_loc.loc.offset == 15);
        REQUIRE(p.current_loc.loc.line == 2);
        REQUIRE(p.current_loc.loc.column == 3);


        /* ----- unterminated --- */
        p =  parser("  /*     X");
        REQUIRE(not p.skip());
        REQUIRE(p.eoi());
        REQUIRE(p.error_list.size() > 0);

        /* --- Multiline ---- */
        /* also make sure embedded openers don't cause problems */
        p = parser("  /*   /*  \n This \n has // \n\tstuff \n /* */\n  :   /*\n */");
        REQUIRE(p.skip());
        REQUIRE(p.current_loc.sv[0] == ':');
        REQUIRE(p.current_loc.loc.offset == 45);
        REQUIRE(p.current_loc.loc.line == 6);
        REQUIRE(p.current_loc.loc.column == 3);

    }

    SECTION("C++ style comments are handled") {
        /* ---- Simple comment --- */
        auto p = parser("  //  Stuff in the comment    \n  X\n\n     ");
        REQUIRE(p.skip());
        REQUIRE(p.current_loc.sv[0] == 'X');
        REQUIRE(p.current_loc.loc.offset == 33);
        REQUIRE(p.current_loc.loc.line == 2);
        REQUIRE(p.current_loc.loc.column == 3);

        /* ----- unterminated --- */
        p =  parser("  //     X");
        REQUIRE(not p.skip());
        REQUIRE(p.eoi());
        REQUIRE(p.error_list.size() > 0);

        /* C inside C++ */
        p = parser("  // /* // \n */\n X ");
        REQUIRE(p.skip());
        REQUIRE(p.current_loc.sv[0] == '*');
        REQUIRE(p.current_loc.loc.offset == 13);
        REQUIRE(p.current_loc.loc.line == 2);
        REQUIRE(p.current_loc.loc.column == 2);
    }
}

TEST_CASE("match_keyword()", "[parser]") {

    SECTION("positive") {
         auto p =  parser("keyword(");
         REQUIRE(p.match_keyword("keyword"));
    }

    SECTION("negative") {
        auto p = parser("keyword_ ");
        REQUIRE(not p.match_keyword("keyword"));
    }
}

TEST_CASE("expect_keyword()", "[parser]") {

    SECTION("positive") {
         auto p =  parser("keyword(");
         REQUIRE(p.expect_keyword("keyword"));
    }

    SECTION("negative") {
        auto p = parser("keyword_ ");
        REQUIRE(not p.expect_keyword("keyword"));
        REQUIRE(p.error_list.size() == 1);
    }
}

TEST_CASE("match_identifier()", "[parser]") {

    SECTION("positive") {
        auto p =  parser("__foo__bar123_ ");
        auto [success, lexeme] = p.match_identifier();
        REQUIRE(success);
        REQUIRE(lexeme == "__foo__bar123_");

        {
        auto p = parser("ruleabc");
        auto [success, lexeme] = p.match_identifier();
        REQUIRE(success);
        REQUIRE(lexeme == "ruleabc");
        }
    }

    SECTION("negative") {
        auto p = parser("rule");
        auto [success, lexeme] = p.match_identifier();
        REQUIRE(not success);
    }
}

TEST_CASE("match_char()", "[parser]") {
    SECTION("positive") {
        auto p = parser("xabcde");
        REQUIRE(p.match_char('x'));
        REQUIRE(p.current_loc.loc.offset == 1);
        REQUIRE(p.match_char('a'));
    }

    SECTION("negative") {
        auto p = parser("");
        REQUIRE(not p.match_char(':'));
        {
        auto p = parser("z");
        REQUIRE(not p.match_char('Z'));
        }
    }
}

TEST_CASE("match_singlequote()", "[parser]") {
    SECTION("positive") {
        {
        auto p = parser("'xyz\\' \"'");
        auto [success, lexeme] = p.match_singlequote();
        REQUIRE(success);
        REQUIRE(lexeme == "'xyz\\' \"'");
        REQUIRE(p.eoi());
        REQUIRE(p.current_loc.loc.offset == 9);
        }
        {
        auto p = parser("'xyz\\\nabc' cccc");
        auto [success, lexeme] = p.match_singlequote();
        REQUIRE(success);
        REQUIRE(lexeme == "'xyz\\\nabc'");
        REQUIRE(not p.eoi());
        REQUIRE(p.current_loc.loc.line == 2);
        REQUIRE(p.current_loc.loc.column == 5);
        REQUIRE(p.current_loc.loc.offset == 10);
        }
    }
    SECTION("negative", "[parser]") {
        // the function actually return 'true' but it counts as an error.
        {
        auto p = parser("'abc\nxyz'");
        auto [success, lexeme] = p.match_singlequote();
        REQUIRE(success);
        REQUIRE(p.error_list.size() == 1);
        }
        {
        auto p = parser("'xyz");
        auto [success, lexeme] = p.match_singlequote();
        REQUIRE(success);
        REQUIRE(p.error_list.size() == 1);
        }

    }
}

TEST_CASE("match_regex()", "[parser]") {
    SECTION("positive") {
        auto p = parser("r:abc[\"\\ ]: ");
        auto [success, lexeme] = p.match_regex();
        REQUIRE(success);
        REQUIRE(lexeme == "r:abc[\"\\ ]:");
        REQUIRE(not p.eoi());
        REQUIRE(p.current_loc.loc.column == 12);
        REQUIRE(p.current_loc.loc.offset == 11);
        {
        auto p = parser(R"d(r:\s+ )d");
        auto [success, lexeme] = p.match_regex();
        REQUIRE(success);
        REQUIRE(lexeme == "r:\\s+");
        REQUIRE(not p.eoi());
        REQUIRE(p.current_loc.loc.column == 6);
        REQUIRE(p.current_loc.loc.offset == 5);
        }
    }
    SECTION("negative") {
        auto p = parser("x:abc[\"\\ ]: ");
        auto [success, lexeme] = p.match_regex();
        REQUIRE(not success);
        REQUIRE(p.current_loc.loc.offset == 0);
    }
}
