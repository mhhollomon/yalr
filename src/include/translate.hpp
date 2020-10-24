#if ! defined(YALR_TRANSLATE_HPP)
#define YALR_TRANSLATE_HPP

#include "analyzer_tree.hpp"
#include "cli_options.hpp"

namespace yalr::translate {

    struct grammophone {
        void output(const yalr::analyzer_tree& gr, cli_options &opts) const;
    };

    struct lexer_graph {
        void output(const yalr::analyzer_tree& gr, cli_options &opts) const;
    };


} // namespace yalr::translate


#endif
