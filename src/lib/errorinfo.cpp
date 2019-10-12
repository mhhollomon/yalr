#include "errorinfo.hpp"

namespace yalr {

    std::ostream& error_info::output(std::ostream& strm) const {

        auto li = fragment.location.line_info();

        strm << fragment.location.source->name << ":" << 
            li.line_num << ':' <<
            li.col_num << ": error:" <<
            message << "\n";
        //
        // put out the formated line
        //
        strm << fragment.location.source->content.substr(
                li.line_start, li.line_end-li.line_start);
        if (fragment.location.source->content[li.line_end] != '\n')
            strm << "\n";
        std::string filler(li.col_num-1, '~');
        strm << filler << "^\n";

        return strm;

    }
} // namepsace yalr
