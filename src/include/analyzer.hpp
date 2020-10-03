#if !defined(YALR_ANALYZER_HPP_)
#define YALR_ANALYZER_HPP_

#include "analyzer_tree.hpp"
#include "parse_tree.hpp"

namespace yalr::analyzer {

    std::unique_ptr<yalr::analyzer_tree> analyze(yalr::parse_tree &tree);

    void pretty_print(const yalr::analyzer_tree &tree, std::ostream& strm);

}

#endif
