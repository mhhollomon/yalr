#include "rpn.hpp"

#include <ostream>
void dump_list(std::ostream &strm, rpn_ptr ptr) {
   for (auto const & [opcode, op1, op2] : *ptr) {
      switch(opcode) {
         case rpn_opcode_t::range :
            strm << "range  " << op1 << ", " << op2 << "\n";
            break;
         case rpn_opcode_t::close :
            strm << "close\n";
            break;
         case rpn_opcode_t::nclose :
            strm << "nclose\n";
            break;
         case rpn_opcode_t::plus :
            strm << "plus\n";
            break;
         case rpn_opcode_t::nplus :
            strm << "nplus\n";
            break;
         case rpn_opcode_t::optn :
            strm << "optn\n";
            break;
         case rpn_opcode_t::noptn :
            strm << "noptn\n";
            break;
         case rpn_opcode_t::concat :
            strm << "concat\n";
            break;
         case rpn_opcode_t::join :
            strm << "join\n";
            break;
         default :
            strm << "unknown  " << op1 << ", " << op2 << "\n";
            break;
      }

   }
}
