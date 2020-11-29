#pragma once

#include "rpn.hpp"

#include <string_view>

#include "sourcetext.hpp"
#include "errorinfo.hpp"

#include <ostream>

enum class rpn_mode {
    full_regex,
    simple_string,
};

rpn_ptr regex2rpn(yalr::text_fragment const &regex, rpn_mode mode, yalr::error_list &errors);

void dump_rpn(std::ostream &strm, rpn_ptr rpn);
