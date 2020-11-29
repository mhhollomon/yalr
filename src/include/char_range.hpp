#pragma once

struct char_range {
    char low;
    char high;

    char_range(char c) : low(c), high(c) {};
    char_range() = default;
    char_range(char l, char h) : low(l), high(h) {};

    bool operator<(char_range const & o) const {
        return (low < o.low or (low == o.low and high < o.high));
    }
};
