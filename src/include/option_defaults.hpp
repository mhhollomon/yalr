#if not defined(YALR_DEFAULT_OPTIONS_HPP)
#define YALR_DEFAULT_OPTIONS_HPP

#include "options.hpp"

namespace yalr {

    constexpr static inline default_setting option_defaults[] = {
        { options::PARSER_CLASS, "Parser"sv },
        { options::LEXER_CLASS,  "Lexer"sv },
        { options::NAMESPACE,    "YalrParser"sv },
        { options::LEXER_CASE,   "cmatch"sv },
    }; 

} // namespace yalr

#endif
