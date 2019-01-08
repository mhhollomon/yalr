#if ! defined(YALR_CODEGEN_HPP)
#define YALR_CODEGEN_HPP

#include "tablegen.hpp"

namespace yalr { namespace codegen {


    void generate_code(const tablegen::lrtable& lt, std::ostream& outstrm);

}}

#endif
