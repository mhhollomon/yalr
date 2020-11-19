#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#define DOCTEST_CONFIG_NO_EXCEPTIONS_BUT_WITH_ALL_ASSERTS
#include "doctest.h"

#include "regex.cpp"

TEST_CASE("[regex] - 'abc'") {
    auto p = regex_parser{"abc"};

    p.parse_regex();

    CHECK(p.success);
    CHECK(p.oper_list_.size() == 5);

    p.dump_list(std::cout);
}
TEST_CASE("[regex] - 'a|b'") {
    auto p = regex_parser{"a|b"};

    p.parse_regex();

    CHECK(p.success);
    CHECK(p.oper_list_.size() == 3);

    p.dump_list(std::cout);
}
TEST_CASE("[regex] - 'ab*'") {
    auto p = regex_parser{"ab*"};

    p.parse_regex();

    CHECK(p.success);
    CHECK(p.oper_list_.size() == 4);

    p.dump_list(std::cout);
}

/***********************************************************/

TEST_CASE("[compiler] - 'abc'") {
    auto p = regex_parser{"abc"};

    p.parse_regex();

    auto comp = regex_compiler{};
    comp.compile_regex(p.oper_list_);

    comp.dump_output(std::cout);
    CHECK(comp.program_.size() == 4);
}
TEST_CASE("[compiler] - 'a|b'") {
    auto p = regex_parser{"a|b"};

    p.parse_regex();
    auto comp = regex_compiler{};
    comp.compile_regex(p.oper_list_);

    comp.dump_output(std::cout);
    CHECK(comp.program_.size() == 6);
}
TEST_CASE("[compiler] - 'ab*'") {
    auto p = regex_parser{"ab*"};

    p.parse_regex();
    auto comp = regex_compiler{};
    comp.compile_regex(p.oper_list_);

    comp.dump_output(std::cout);
    CHECK(comp.program_.size() == 6);
}
TEST_CASE("[compiler] - 'a+b+'") {
    auto p = regex_parser{"a+b+"};

    p.parse_regex();
    auto comp = regex_compiler{};
    comp.compile_regex(p.oper_list_);

    comp.dump_output(std::cout);
    CHECK(comp.program_.size() == 7);
}
TEST_CASE("[compiler] - '(a(b?)c)?") {
    auto p = regex_parser{"(a(b?)c)?"};

    p.parse_regex();
    std::cout << "------ parse -----\n";
    p.dump_list(std::cout);

    auto comp = regex_compiler{};
    comp.compile_regex(p.oper_list_);

    std::cout << "---- program ----\n";
    comp.dump_output(std::cout);
    CHECK(comp.program_.size() == 8);
}

/***********************************************************/

TEST_CASE("[runner] - 'abc'") {
    auto p = regex_parser{"abc"};

    p.parse_regex();

    auto comp = regex_compiler{};
    comp.compile_regex(p.oper_list_);

    comp.dump_output(std::cout);

    CHECK(comp.run("abc"));
    CHECK(comp.run("abcd"));
    CHECK_FALSE(comp.run("abf"));
}
TEST_CASE("[runner] - '((a|b)?c+)+") {
    auto p = regex_parser{"((a|b)?c+)+"};

    p.parse_regex();

    auto comp = regex_compiler{};
    comp.compile_regex(p.oper_list_);

    comp.dump_output(std::cout);

    CHECK(comp.run("acbcc"));
    CHECK(comp.run("ccccacbc"));

}
TEST_CASE("[runner] - '<.*>'") {
    auto p = regex_parser{"<.*>"};

    p.parse_regex();

    auto comp = regex_compiler{};
    comp.compile_regex(p.oper_list_);

    comp.dump_output(std::cout);

    CHECK(comp.run("<html>"));
    CHECK(comp.run("<html></html>"));

}
TEST_CASE("[runner] - '<.*?>'") {
    auto p = regex_parser{"<.*?>"};

    p.parse_regex();

    auto comp = regex_compiler{};
    comp.compile_regex(p.oper_list_);

    comp.dump_output(std::cout);

    CHECK(comp.run("<html>"));
    CHECK(comp.run("<html></html>"));
    CHECK(comp.match_length == 6);
}
TEST_CASE("[runner] - '\'\\\x23\\xg\\x'") {
    auto p = regex_parser{R"x(\'\\\x23\xg\x)x"};

    p.parse_regex();
    //p.dump_list(std::cout);

    auto comp = regex_compiler{};
    comp.compile_regex(p.oper_list_);

    //comp.dump_output(std::cout);

    CHECK(comp.run("'\\#xgx"));
}

TEST_CASE("[runner] - 'a[bcd]*e") {
    auto p = regex_parser{R"x(a[bdc]*e)x"};

    p.parse_regex();
    //p.dump_list(std::cout);

    auto comp = regex_compiler{};
    comp.compile_regex(p.oper_list_);

    //comp.dump_output(std::cout);

    CHECK(comp.run("abbcdde"));
}
TEST_CASE("[runner] - '[aA-C\\x23\\wa-\\x7f]'") {
    auto p = regex_parser{R"x([aA-C\x23\w\a-\x7f]*)x"};

    p.parse_regex();
    p.dump_list(std::cout);

    auto comp = regex_compiler{};
    comp.compile_regex(p.oper_list_);

    comp.dump_output(std::cout);

    CHECK(comp.run("aB#_z"));
    CHECK(comp.match_length == 5);
}
