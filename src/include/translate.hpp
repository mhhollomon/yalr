#if ! defined(YALR_TRANSLATE_HPP)
#define YALR_TRANSLATE_HPP

#include "analyzer_tree.hpp"
#include "CLIOptions.hpp"

namespace yalr::translate {

    struct grammophone {

        void output(const yalr::analyzer_tree& gr, CLIOptions &opts) const;
    };

} // namespace yalr::translate


#endif
