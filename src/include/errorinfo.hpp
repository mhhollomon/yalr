#if not defined(YALR_ERROR_INFO_HPP)
#define YALR_ERROR_INFO_HPP

#include "sourcetext.hpp"

#include "constants.hpp"

#include <list>
#include <iostream>

namespace yalr {

    struct error_info;

    struct error_list {
        std::list<error_info> errors;

        error_info &add(const std::string& msg, text_fragment tf, message_type mt=message_type::error) {
            errors.emplace_back(msg, tf, mt); 

            return errors.back();
        }

        int size() const { return static_cast<int>(errors.size()); }

        std::ostream& output(std::ostream &strm) const;
    };

    struct error_info {
        std::string message;
        text_fragment fragment;
        error_list aux_info;
        message_type msg_type;

        error_info(const std::string & msg, text_fragment frag, message_type mt=message_type::error) :
            message(msg), fragment(frag),
            msg_type{mt}
        {}

        error_info() = default;
        error_info(const yalr::error_info& o) = default;
        error_info(yalr::error_info&& o) = default;

        void add_info(const std::string& msg, text_fragment tf) {
            aux_info.add(msg, tf, message_type::info);
        }

        std::ostream& output(std::ostream &strm) const;
    };
} // namespace yalr

#endif
