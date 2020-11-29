#pragma once

#include "char_range.hpp"
#include <set>

std::set<char_range> unlap_char_ranges(std::set<char_range> const & wood_pile);
