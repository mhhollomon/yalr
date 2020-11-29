#pragma once

#include <cstdint>
#include <list>
#include <vector>
#include <memory>

enum class vm_opcode_t : int16_t {
    // no operation - no operands
    noop,

    // Match literal - op[0] = character
    literal,

    // Match char range - op[0] = low, op[1] = high
    range,

    // spawn two threads to replace current
    // op[0], op[1] = the two program counters
    split,

    // jump to new program counter - op[0] = new pc
    jump,

    // Signal success - op[0] = id of lexical unit
    match,
};

struct vm_instruction {
    vm_opcode_t opcode;

    // Nothing uses op[2] (yet).
    int16_t op[3];

};

struct vm_program : std::shared_ptr<std::vector<vm_instruction>> {
    using std::shared_ptr<std::vector<vm_instruction>>::shared_ptr;

    vm_program() {
        auto tmp = std::make_shared<std::vector<vm_instruction>>();
        std::swap(*this, tmp);
    }

    vm_instruction operator[](int offset) {
        return (*this)->at(offset);
    }

    vm_instruction at(int offset) {
        return (*this)->at(offset);
    }

    int size() {
        return int((*this)->size());
    }
};

struct vm_fragment {
    vm_program code;

    auto add_code(vm_opcode_t opcode, int16_t op0 = 0, int16_t op1 = 0, int16_t op2 = 0) {
        return code->emplace_back(vm_instruction{opcode, { op0, op1, op2 }});
    }

    void append(vm_fragment const &o) {
        auto last_pc = code->size();
        code->insert(code->end(), o.code->cbegin(), o.code->cend());
        for (auto pc = last_pc ; pc < code->size() ; ++pc) {
            auto &instr = code->at(pc);
            switch (instr.opcode) {
                case vm_opcode_t::jump :
                    instr.op[0] += last_pc;
                    break;
                case vm_opcode_t::split :
                    instr.op[0] += last_pc;
                    instr.op[1] += last_pc;
                    break;
                default :
                    /* do nothing */
                    break;
            }
        }
    }

    auto size() { return code.size(); }
};
