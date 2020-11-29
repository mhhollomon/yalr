#pragma once

#include "vm.hpp"
#include "rpn.hpp"

#include <memory>

vm_program rpn2vm(rpn_ptr rpn);

void dump_vm_program(std::ostream &strm, vm_program program_);
