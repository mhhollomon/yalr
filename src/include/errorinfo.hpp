#if not defined(YALR_ERROR_INFO_HPP)
#define YALR_ERROR_INFO_HPP

#include "sourcetext.hpp"
#include <list>
#include <iostream>

namespace yalr {

    struct error_info {
        std::string message;
        text_fragment fragment;
        std::list<error_info> aux_info;

        error_info(const std::string & msg, text_fragment frag) :
            message(msg), fragment(frag) {}

        error_info() = default;
        error_info(const yalr::error_info& o) = default;
        error_info(yalr::error_info&& o) = default;


        std::ostream& output(std::ostream &strm) const;
    };
} // namespace yalr

#endif
