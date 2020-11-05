#pragma once

#include "codegen/fsm.hpp"

#include <memory>

namespace yalr::codegen {

    std::unique_ptr<nfa_machine> build_nfa(const symbol_table &sym_tab);

    std::unique_ptr<dfa_machine> build_dfa(const nfa_machine &nfa);

} // yalr::codegen
