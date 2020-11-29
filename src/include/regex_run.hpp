#pragma once

#include "vm.hpp"

#include <string_view>
#include <iostream>

using run_results = std::pair<bool, int>;

struct regex_runner {
    vm_program program_;
    std::string_view regex_;

    regex_runner(vm_program p, std::string_view regex) : program_(p), regex_(regex) {}
    regex_runner(vm_program p) : program_(p), regex_("unknown") {}
    regex_runner() = default;

    run_results match(std::string_view txt) const;
};
