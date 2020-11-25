#pragma once

#include <cstdint>


enum class vm_opcode_t : int16_t {
    // no operation - no operands
    noop,

    // Match literal - op1 = character
    literal,

    // Match char range - op1 = low, op2 = high
    range,

    // spawn two threads to replace current
    // op1, op2 = the two program counters
    split,

    // jump to new program counter - op1 = new pc
    jump,

    // Signal success - op1 = id of lexical unit
    match,
};

struct operation {
    vm_opcode_t oper;
    int16_t op1, op2, op3;
};
