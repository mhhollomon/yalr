/*
 * The SymbolTable is not actually a part of the parser nor even used
 * by the parser. But it is a prerequiste for the analyzer. So it seemed
 * to make sense to number it down with the low-level parser tests.
 */

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
//#include <sstream>      // std::stringstream

#include "SymbolTable.hpp"


namespace pgt = yalr::pgt;


TEST_CASE("[symboltable.terminal] Term Handling") {

    yalr::SymbolTable::SymbolTable st;
    pgt::terminal t;
    t.name = "foo";
    t.pattern = R"('keyword')";

    auto [inserted, new_sym] = st.insert(t.name, t);
    REQUIRE(inserted);
    REQUIRE(new_sym.type_name() == "Term");
    REQUIRE(new_sym.name() == "foo");

    SUBCASE("[symboltable.terminal] Can retrieve the full Term data") {
        auto [found, sym] = st.find("foo");
        REQUIRE(found);
        CHECK(sym.type_name() == "Term");
        CHECK(sym.isterm());
        CHECK_FALSE(sym.isrule());
        CHECK_FALSE(sym.isskip());

        auto t = sym.getTerminalInfo();
        CHECK(t != nullptr);

        auto s = sym.getSkipInfo();
        CHECK( s == nullptr );

        auto r = sym.getRuleInfo();
        CHECK( r == nullptr);
    }

    SUBCASE("[symboltable.terminal] two with same name gives an error") {
        pgt::terminal t2;
        t2.name = "foo";
        t2.pattern = R"('abc')";
        auto [inserted, new_sym] = st.insert(t2.name, t2);
        CHECK_FALSE(inserted);
        CHECK(new_sym.name() == "foo");
        CHECK(new_sym.getTerminalInfo()->pattern == "'keyword'");
    }

}

TEST_CASE("[symboltable.skip] Skip Handling") {

    yalr::SymbolTable::SymbolTable st;
    pgt::skip t;
    t.name = "foo";
    t.pattern = R"('keyword')";

    auto [inserted, new_sym] = st.insert(t.name, t);
    REQUIRE(inserted);
    REQUIRE(new_sym.type_name() == "Skip");
    REQUIRE(new_sym.name() == "foo");

    SUBCASE("[symboltable.skip] Can retrieve the full Skip data") {
        auto [found, sym] = st.find("foo");
        REQUIRE(found);
        CHECK(sym.type_name() == "Skip");
        CHECK_FALSE(sym.isterm());
        CHECK_FALSE(sym.isrule());
        CHECK(sym.isskip());

        auto t = sym.getTerminalInfo();
        CHECK(t == nullptr);

        auto s = sym.getSkipInfo();
        CHECK( s != nullptr );

        auto r = sym.getRuleInfo();
        CHECK( r == nullptr);
    }

    SUBCASE("[symboltable.skip] two with same name gives an error") {
        pgt::skip t2;
        t2.name = "foo";
        t2.pattern = R"('abc')";
        auto [inserted, new_sym] = st.insert(t2.name, t2);
        CHECK_FALSE(inserted);
        CHECK(new_sym.name() == "foo");
        CHECK(new_sym.getSkipInfo()->pattern == "'keyword'");
    }

}

TEST_CASE("[symboltable.rule] Rule Handling") {

    yalr::SymbolTable::SymbolTable st;
    pgt::rule_def t;
    t.name = "foo";

    auto [inserted, new_sym] = st.insert(t.name, t);
    REQUIRE(inserted);
    REQUIRE(new_sym.type_name() == "Rule");
    REQUIRE(new_sym.name() == "foo");

    SUBCASE("[symboltable.rule] Can retrieve the full Rule data") {
        auto [found, sym] = st.find("foo");
        REQUIRE(found);
        CHECK(sym.type_name() == "Rule");
        CHECK_FALSE(sym.isterm());
        CHECK_FALSE(sym.isskip());
        CHECK(sym.isrule());

        auto t = sym.getTerminalInfo();
        CHECK(t == nullptr);

        auto s = sym.getSkipInfo();
        CHECK( s == nullptr );

        auto r = sym.getRuleInfo();
        CHECK( r != nullptr);
    }

    SUBCASE("[symboltable.rule] two with same name gives an error") {
        pgt::rule_def t2;
        t2.name = "foo";
        auto [inserted, new_sym] = st.insert(t2.name, t2);
        CHECK_FALSE(inserted);
        CHECK(new_sym.name() == "foo");
    }

}
