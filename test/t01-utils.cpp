#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "utils.hpp"

struct test1_t : public yalr::util::identifier_t<test1_t> { using yalr::util::identifier_t<test1_t>::identifier_t; };
struct test2_t : public yalr::util::identifier_t<test2_t> { using yalr::util::identifier_t<test2_t>::identifier_t; };


TEST_CASE("[utils] - identifier") {

    auto t1_1 = test1_t::get_next_id();
    CHECK(int(t1_1) == 0);
    auto t1_2 = t1_1;
    CHECK(int(t1_2) == 0);
    CHECK(t1_1 == t1_2);
    auto t1_3 = test1_t::get_next_id();
    CHECK(t1_3 > t1_1);

    auto t2_1 = test2_t::get_next_id();
    CHECK(int(t2_1) == 0);
}
