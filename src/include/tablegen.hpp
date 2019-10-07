#if ! defined(YALR_TABLEGEN_HPP)
#define YALR_TABLEGEN_HPP

#include "analyzer_tree.hpp"
#include "lrtable.hpp"

namespace yalr {


    std::unique_ptr<lrtable> generate_table(const analyzer_tree& g);

    
    void pretty_print(const lrtable& lt, std::ostream& strm);

}

#endif
