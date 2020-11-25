#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "regex_parse.hpp"
#include "sourcetext.hpp"

/*
 * NOTE : These tests are not so much to test that the regex are parsed CORRECTLY,
 * but merely that the correct language is recognized.
 *
 * This is all the analyzer stage guarnatees - that the regex is valid.
 * Later stages will need to make sure that the resulting recognizer actually
 * does the correct thing.
 */

rpn_ptr call_regex2rpn(std::string_view txt, rpn_mode m, yalr::error_list &e) {

  auto ts = std::make_shared<yalr::text_source>("testing", std::string(txt));
  return regex2rpn(yalr::text_fragment{txt, yalr::text_location{0, ts}}, m, e);
}

TEST_CASE("[regex2rpn] - simple_string") {
  yalr::error_list el;

  // These should all pass.

  auto retval = call_regex2rpn("[", rpn_mode::simple_string, el);
  CHECK(retval);
  CHECK(el.size() == 0);

  retval = call_regex2rpn("[]", rpn_mode::simple_string, el);
  CHECK(retval);
  CHECK(el.size() == 0);

  retval = call_regex2rpn(")(", rpn_mode::simple_string, el);
  CHECK(retval);
  CHECK(el.size() == 0);

}

TEST_CASE("[regex2rpn] - regex - empty char class") {
  yalr::error_list el;
  auto retval = call_regex2rpn("a[]b", rpn_mode::full_regex, el);
  CHECK(not retval);
  CHECK(el.size() == 1);
}

TEST_CASE("[regex2rpn] - regex - missing close bracket") {
  yalr::error_list el;
  auto retval = call_regex2rpn("a[b", rpn_mode::full_regex, el);
  CHECK(not retval);
  CHECK(el.size() == 1);
}

TEST_CASE("[regex2rpn] - regex - bad range") {
  yalr::error_list el;
  auto retval = call_regex2rpn("a[z-a]", rpn_mode::full_regex, el);
  CHECK(not retval);
  CHECK(el.size() == 1);
}

TEST_CASE("[regex2rpn] - regex - no closing paren") {
  yalr::error_list el;
  auto retval = call_regex2rpn("a((b)*", rpn_mode::full_regex, el);
  CHECK(not retval);
  CHECK(el.size() == 1);
}

TEST_CASE("[regex2rpn] - regex - positive") {
  yalr::error_list el;

  auto retval = call_regex2rpn(R"xx(a\x23.\((\w[A-Z\x23-\x26.[])*?)xx", rpn_mode::full_regex, el);
  CHECK(retval);
  CHECK(el.size() == 0);

  retval = call_regex2rpn("a|", rpn_mode::full_regex, el);
  CHECK(retval);
  CHECK(el.size() == 0);

  retval = call_regex2rpn("|b", rpn_mode::full_regex, el);
  CHECK(retval);
  CHECK(el.size() == 0);

}
