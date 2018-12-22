#if !defined(YALR_GRAMMAR_HPP)
#define YALR_GRAMMAR_HPP

#include "ast.hpp"

#include <boost/config/warning_disable.hpp>
#include <boost/spirit/home/x3.hpp>
#include <boost/spirit/include/support_istream_iterator.hpp>

namespace yalr
{
    namespace parser
    {
        using iterator_type = boost::spirit::istream_iterator;
        using ast_tree_type = boost::spirit::x3::unused_type;
    } 
    std::pair<bool, parser::ast_tree_type>do_parse(
            parser::iterator_type &first, parser::iterator_type const &last);
}

#endif
