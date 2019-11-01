#include "errorinfo.hpp"

namespace yalr {

    const std::string level[] = {
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

        strm << fragment.location.source->name << ":" << 
            li.line_num << ':' <<
            li.col_num << ": " << level[int(msg_type)] << ": " <<
            message << "\n";
        //
        // put out the formatted line
        //
        strm << fragment.location.source->content.substr(
                li.line_start, li.line_end-li.line_start+1);
        if (fragment.location.source->content[li.line_end] != '\n')
            strm << "\n";
        std::string filler(li.col_num-1, '~');
        strm << filler << "^\n";

        //
        // Now the nested infos
        //
        aux_info.output(strm);

        return strm;

    }
} // namepsace yalr
