#if ! defined(YALR_TRANSLATE_HPP)
#define YALR_TRANSLATE_HPP

#include "analyzer.hpp"

namespace yalr::translate {

    struct grammophone {
        const analyzer::grammar& g;

        grammophone(const analyzer::grammar& gr) : g(gr) {};

        void output(std::ostream& strm) const;
    };

} // namespace yalr::translate


#endif
