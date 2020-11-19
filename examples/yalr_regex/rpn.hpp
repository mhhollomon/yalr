#pragma once

#include <list>
#include <memory>

enum class rpn_opcode_t {
    range,
    close,
    nclose,
    plus,
    nplus,
    optn,
    noptn,
    concat,
    join,
};

struct operation {
    rpn_opcode_t oper;
    int op1;
    int op2;
};

using rpn = std::list<operation>;
using rpn_ptr = std::shared_ptr<rpn>;

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

void dump_list(std::ostream &strm, rpn_ptr ptr);
