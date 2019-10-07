#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "sourcetext.hpp"


#include <string>
#include <memory>

std::string s = "This is a test";

TEST_CASE("[sourcetext]") {

    auto ts = new yalr::text_source("/a/b", std::move(s));

    CHECK(ts->name == "/a/b");
}
