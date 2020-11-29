#include "utils.hpp"
#include <cctype>
#include <iomanip>
#include <sstream>
#include <ostream>

namespace yalr::util {

std::string escape_char(char c, bool use_char_name) {
    switch (c) {
        case '\0' :
            return use_char_name ? "{NUL}" : "\\0";
        case ' ' :
            return use_char_name ? "{SPC}" : " ";
        case '\\' :
            return "\\\\";
        case '\'' :
            return "\\'";
        case '\a' :
            return use_char_name ? "{BEL}" : "\\a";
        case '\b' :
            return use_char_name ? "{BS}" : "\\b";
        case '\f' :
            return use_char_name ? "{FF}" : "\\f";
        case '\n' :
            return use_char_name ? "{LF}" : "\\n";
        case '\r' :
            return use_char_name ? "{CR}" : "\\r";
        case '\t' :
            return use_char_name ? "{HT}" : "\\t";
        case '\v' :
            return use_char_name ? "{VT}" : "\\v";
    }

    if (isprint(c)) {
        return std::string{c};
    } else {
        std::stringstream ss;
        ss << "\\x" << std::setw(2) << std::setfill('0') << std::hex << int(c);

        return ss.str();
    }

}

std::string escape_string(std::string_view v, bool use_char_name) {
    std::ostringstream ss;

    for (auto const x : v) {
        ss << escape_char(x);
    }

    return ss.str();
}


} // namespsace yalr::util
