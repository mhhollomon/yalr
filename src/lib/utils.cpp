#include "utils.hpp"
#include <cctype>
#include <iomanip>

namespace yalr::util {

std::string escape_char(char c) {
    switch (c) {
        case '\0' :
            return "\\0";
        case '\'' :
            return "\\'";
        case '\a' :
            return "\\a";
        case '\b' :
            return "\\b";
        case '\f' :
            return "\\f";
        case '\n' :
            return "\\n";
        case '\r' :
            return "\\r";
        case '\t' :
            return "\\t";
        case '\v' :
            return "\\v";
    }

    if (isprint(c)) {
        return std::string{c};
    } else {
        std::stringstream ss;
        ss << "\\x" << std::setw(2) << std::setfill('0') << std::hex << int(c);

        return ss.str();
    }

}


} // namespsace yalr::util
