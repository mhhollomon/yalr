#if ! defined(YALR_TRANSLATE_HPP)
#define YALR_TRANSLATE_HPP

#include "analyzer.hpp"
#include "CLIOptions.hpp"

namespace yalr::translate {

    struct grammophone {

        void output(const analyzer::grammar& gr, CLIOptions &opts) const;
    };

} // namespace yalr::translate


#endif
