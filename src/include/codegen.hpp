#if ! defined(YALR_CODEGEN_HPP)
#define YALR_CODEGEN_HPP

#include "tablegen.hpp"

namespace yalr {


    void generate_code(const lrtable& lt, std::ostream& outstrm);

} // namespace yalr

#endif
