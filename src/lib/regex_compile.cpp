#include "regex_compile.hpp"

#include "utils.hpp"

#include <deque>
#include <ostream>
#include <iomanip>

using yalr::util::escape_char;

struct regex_compiler {

    vm_program program_;

    std::deque<vm_fragment> stack_;

    int last_label_ = 0;



    vm_fragment stack_opcode(vm_opcode_t oper, 
            int16_t op1 = 0, int16_t op2 = 0, int16_t op3 = 0) {
        auto &iter = stack_.emplace_back();
        iter.add_code(oper, op1, op2, op3);
        return iter;
    }

    vm_program compile_regex(rpn_ptr parse) {
        for (auto const &oper : *parse) {
            switch(oper.opcode) {
                case rpn_opcode_t::literal :
                    stack_opcode(vm_opcode_t::literal, oper.op1);
                    break;

                case rpn_opcode_t::range :
                    stack_opcode(vm_opcode_t::range, oper.op1, oper.op2);
                    break;

                case rpn_opcode_t::concat : {
                    // e1e2 :
                    //      e1
                    //      e2
                    //
                    auto second = stack_.back(); stack_.pop_back();
                    auto first  = stack_.back(); stack_.pop_back();

                    first.append(second);

                    stack_.emplace_back(std::move(first));
                    }
                    break;
                case rpn_opcode_t::join : {
                    // e1 | e2 :
                    //      split 1 , 2
                    //      1: e1
                    //      jmp 3
                    //      2 : e2
                    //      3 : 
                    //
                    auto second = stack_.back(); stack_.pop_back();
                    auto first  = stack_.back(); stack_.pop_back();

                    auto results = vm_fragment();

                    // The + 2 is to account for th fact that the split is
                    // in front and jump will be in the back
                    results.add_code(vm_opcode_t::split, 1, first.size() + 2);
                    results.append(first);
                    results.add_code(vm_opcode_t::jump, 
                            first.size() + 2 + second.size());
                    results.append(second);
                    stack_.emplace_back(std::move(results));
                    }
                    break;
                case rpn_opcode_t::nclose :
                case rpn_opcode_t::close : {
                    // e1* :
                    //      1 : split 2, 3
                    //      2 : e1
                    //          jump 1
                    //      3: 
                    auto first  = stack_.back(); stack_.pop_back();

                    auto results = vm_fragment();

                    // The + 2 is to account for the fact that the split is
                    // in front and jump will be in the back
                    if (oper.opcode == rpn_opcode_t::close) {
                        results.add_code(vm_opcode_t::split, 1, first.size() + 2);
                    } else {
                        // non-greedy - prioritize the null case
                        results.add_code(vm_opcode_t::split, first.size() + 2, 1);
                    }

                    results.append(first);
                    results.add_code(vm_opcode_t::jump,  0);

                    stack_.emplace_back(std::move(results));

                    }

                    break;
                case rpn_opcode_t::nplus :
                case rpn_opcode_t::plus : {
                    // e1+ :
                    //      1: e1
                    //         split 1, 2
                    //      2: 
                    auto first  = stack_.back(); stack_.pop_back();

                    if (oper.opcode == rpn_opcode_t::plus) {
                        first.add_code(vm_opcode_t::split, 0, first.size() + 1);
                    } else {
                        first.add_code(vm_opcode_t::split, first.size() + 1, 0);
                    }

                    stack_.emplace_back(std::move(first));
                    }
                    break;
                case rpn_opcode_t::noption :
                case rpn_opcode_t::option : {
                    // e1? :
                    //         split 1, 2
                    //      1: e1
                    //      2: 
                    auto first  = stack_.back(); stack_.pop_back();

                    auto results = vm_fragment();
                    
                    if (oper.opcode == rpn_opcode_t::option) {
                        results.add_code(vm_opcode_t::split, 1, first.size()+1);
                    } else {
                        results.add_code(vm_opcode_t::split, first.size()+1, 1);
                    }
                    results.append(first);

                    stack_.emplace_back(std::move(results));
                    }
                    break;
                default :
                    std::cerr << "Not implemented yet\n";
                    return nullptr;
            }
        }

        auto last = stack_.back(); stack_.pop_back();
        last.add_code(vm_opcode_t::match);
        return last.code;

    }

};

/****************************************************************/
vm_program rpn2vm(rpn_ptr rpn) {
    regex_compiler compiler;
    compiler.compile_regex(rpn);

    return compiler.compile_regex(rpn);
}

void dump_vm_program(std::ostream &strm, vm_program program_) {
    strm << "--------\n";
    int pc = -1;
    for (auto const &oper : *program_) {
        ++pc;
        strm << std::setw(2) << pc << ": ";
        switch (oper.opcode) {
            case vm_opcode_t::noop :
                strm << "noop\n";
                break;
            case vm_opcode_t::literal :
                strm << "litrl '" << 
                    escape_char(static_cast<char>(oper.op[0])) << "'\n";
                break;
            case vm_opcode_t::range :
                strm << "range '" << 
                    escape_char(static_cast<char>(oper.op[0])) << "', '";
                strm << escape_char(static_cast<char>(oper.op[1])) << "'\n";
                break;
            case vm_opcode_t::split :
                strm << "split " << oper.op[0] << ", " << oper.op[1] << "\n";
                break;
            case vm_opcode_t::jump :
                strm << "jump  " << oper.op[0] << "\n";
                break;
            case vm_opcode_t::match :
                strm << "match\n";
                break;
            default :
                strm << "unkown " << oper.op[0] << ", " << oper.op[1] << "\n";
        }
    }
    strm << "--------\n";
}
