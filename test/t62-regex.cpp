#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#define DOCTEST_CONFIG_NO_EXCEPTIONS_BUT_WITH_ALL_ASSERTS
#include "doctest.h"

#include "regex_parse.hpp"
#include "regex_compile.hpp"
#include "regex_run.hpp"
#include "utils.hpp"

#include <sstream>
#include <iostream>

using yalr::util::escape_string;

vm_program compile_regex(std::string_view regex, rpn_mode mode = rpn_mode::full_regex,
        bool do_rpn_dump = false ) {
    yalr::error_list errors;
    auto ts = std::make_shared<yalr::text_source>("testing", std::string(regex));
    auto rpn =  regex2rpn(
            yalr::text_fragment{regex, yalr::text_location{0, ts}}, mode, errors);
    if (not rpn) {
        errors.output(std::cerr);
        return nullptr;
    }

    if (do_rpn_dump) {
        std::cout << "RPN for '" << regex << "' :\n";
        dump_rpn(std::cout, rpn);
    }

    return rpn2vm(rpn);
}
   


TEST_CASE("[compiler] - 'abc'") {
    auto prog = compile_regex("abc", rpn_mode::simple_string);
    CHECK(prog);

    //dump_vm_program(std::cout, prog);
    CHECK(prog.size() == 4);
}
TEST_CASE("[compiler] - 'a|b'") {
    auto prog = compile_regex("a|b");
    CHECK(prog);

    //dump_vm_program(std::cout, prog);
    CHECK(prog.size() == 5);
}
TEST_CASE("[compiler] - 'ab*'") {
    auto prog = compile_regex("ab*");
    CHECK(prog);

    //dump_vm_program(std::cout, prog);
    CHECK(prog.size() == 5);
}
TEST_CASE("[compiler] - 'a+b+'") {
    auto prog = compile_regex("a+b+");
    CHECK(prog);

    //dump_vm_program(std::cout, prog);
    CHECK(prog.size() == 5);
}
TEST_CASE("[compiler] - '(a(b?)c)?") {
    auto prog = compile_regex("(a(b?)c)?", rpn_mode::full_regex);
    CHECK(prog);

    //dump_vm_program(std::cout, prog);
    CHECK(prog.size() == 6);
}

/*****************************************************************************/

namespace std {
    doctest::String toString(const run_results &value) {
        std::ostringstream ss;

        ss << "pair<" << value.first << ", " << value.second << '>';
        return ss.str().c_str();
    }
}

/*****************************************************************************/
struct test_run {
    vm_program program_;
    std::string_view regex_;
    regex_runner runner_;

    test_run(std::string_view regex, rpn_mode mode = rpn_mode::full_regex) :regex_(regex) {
        program_ = compile_regex(regex_, mode);
        REQUIRE(program_);
        runner_ = regex_runner{program_};
    }
    void check_good_match(std::string_view text, int length) {
        CAPTURE(regex_);
        INFO("text := " << escape_string(text)); 
        CAPTURE(length);
        auto retval = runner_.match(text);
        CAPTURE(retval.first);
        CAPTURE(retval.second);
        CHECK(retval == std::make_pair(true, length));
    }
    void check_bad_match(std::string_view text) {
        CAPTURE(regex_);
        INFO("text := " << escape_string(text)); 
        auto retval = runner_.match(text);
        CAPTURE(retval.first);
        CAPTURE(retval.second);
        CHECK(retval == std::make_pair(false, 0));
    }
};
/*****************************************************************************/

TEST_CASE("[runner] - 'abc'") {
    auto test = test_run{"abc"};

    test.check_good_match("abc", 3);
    test.check_good_match("abcd", 3);
    test.check_bad_match("abf");
    test.check_bad_match("a");
}
TEST_CASE("[runner] - '.' (simple)") {
    auto test = test_run{".", rpn_mode::simple_string};

    test.check_good_match(".", 1);
    test.check_bad_match("a");
}
TEST_CASE("[runner] - '.' (char class)") {
    auto test = test_run{".", rpn_mode::full_regex};

    test.check_good_match(".", 1);
    test.check_good_match("a", 1);
    test.check_good_match("\x7f", 1);
    test.check_bad_match("\n");
}

TEST_CASE("[runner] - '((a|b)?c+)+") {
    auto test = test_run{"((a|b)?c+)+"};

    test.check_good_match("acbcc", 5);
    test.check_good_match("ccccacbc", 8);
}
TEST_CASE("[runner] - '<.*>'") {
    std::string regex = "<.*>";

    auto test = test_run{regex};

    test.check_good_match("<html>", 6);
    test.check_good_match("<html></html>", 13);
}
TEST_CASE("[runner] - '<.*?>'") {
    std::string regex = "<.*?>";

    auto test = test_run{regex};

    test.check_good_match("<html>", 6);
    test.check_good_match("<html></html>", 6);
}

TEST_CASE("[runner] - '\'\\\x23\\xg\\x'") {
    std::string regex = R"x(\'\\\x23\xg\x)x";

    auto test = test_run{regex};

    test.check_good_match("'\\#xgx", 6);
}

TEST_CASE("[runner] - 'a[bcd]*e") {
    std::string regex = R"x(a[bdc]*e)x";

    auto test = test_run{regex};

    test.check_good_match(("abbcdde"), 7);
}
TEST_CASE("[runner] - '[aA-C\\x23\\wa-\\x7f]'") {
    std::string regex = R"x([aA-C\x23\w\a-\x7f]*)x";

    auto test = test_run{regex};

    test.check_good_match("aB#_z\x70", 6);
}
TEST_CASE("[runner] - '[^a-dF-I]+'") {
    std::string regex = "[^a-dF-I]+";

    auto test = test_run{regex};

    //dump_vm_program(std::cout, test.program_);

    test.check_good_match("eAZ\x7F\x01\n", 6);
    test.check_bad_match("c");
    test.check_bad_match("G");
}
