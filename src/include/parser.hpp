#if not defined(YALR_PARSER_HPP)
#define YALR_PARSER_HPP

#include "parse_tree.hpp"

#include <memory>

namespace yalr {

    struct parser_guts;

    class yalr_parser {
        std::unique_ptr<parser_guts> guts;

      public:
        yalr_parser(std::shared_ptr<text_source> source);
        ~yalr_parser();
        
        //
        // Pretty print the errors to the stream provided.
        //
        std::ostream& stream_errors(std::ostream& ostrm);

        //
        // Top Level parser
        // Entry point
        //
        parse_tree& parse();

        
    }; // yalr_parser

} // namespace yalr

#endif
