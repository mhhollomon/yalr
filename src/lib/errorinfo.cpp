#include "errorinfo.hpp"

#include <array>

namespace yalr {

    const std::array<std::string, 3> level = {
        "info", "warning", "error"
    };

    std::ostream& error_list::output(std::ostream& strm) const {
        for (auto const & e : errors) {
            e.output(strm);
        }
        return strm;
    }

    std::ostream& error_info::output(std::ostream& strm) const {

        auto li = fragment.location.line_info();

        //
        // Put out the actual error message
        //
        strm << fragment.location.source->name << ":" << 
            li.line_num << ':' <<
            li.col_num << ": " << level.at(int(msg_type)) << ": " <<
            message << "\n";
        //
        // put out the source line
        //
        strm << fragment.location.source->content.substr(
                li.line_start, li.line_end-li.line_start+1);
        if (fragment.location.source->content[li.line_end] != '\n') {
            strm << "\n";
        }


        //
        // Output the attention pointer
        //
        auto text_len = fragment.text.size();

        if (text_len > 0) {
            strm << std::string(size_t(li.col_num-1), ' ') << "^";
            strm << std::string(size_t(text_len-1), '~') << "\n";
        } else if (li.col_num > 1) {
            strm << std::string(size_t(li.col_num-2), ' ') << "~^~";
        } else {
            strm << "^~";
        }
        strm << "\n";

        //
        // Now the nested infos
        //
        aux_info.output(strm);

        return strm;

    }
} // namespace yalr
