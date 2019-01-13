#if !defined(YALR_PARSER_HPP)
#define YALR_PARSER_HPP

#include "ast.hpp"

#include <boost/config/warning_disable.hpp>
#include <boost/spirit/home/x3.hpp>
//#include <boost/spirit/include/support_istream_iterator.hpp>

namespace yalr
{
    namespace parser
    {
        //using iterator_type = boost::spirit::istream_iterator;
        using iterator_type = std::string::const_iterator;
        using ast_tree_type = ast::grammar;
    } 
    parser::ast_tree_type do_parse(
            parser::iterator_type &first, parser::iterator_type const &last);
}

#endif
