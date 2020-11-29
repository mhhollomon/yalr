#pragma once

#include <cstdint>
#include <list>
#include <memory>

enum class rpn_opcode_t : int16_t {
    literal,
    range,
    concat,
    option,
    noption,
    plus,
    nplus,
    close,
    nclose,
    join,
};

struct rpn_instruction {
    rpn_opcode_t opcode;
    int16_t op1, op2, op3;
};

using rpn_list = std::list<rpn_instruction>;
using rpn_ptr = std::shared_ptr<rpn_list>;
