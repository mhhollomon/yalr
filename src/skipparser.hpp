#if !defined(YALR_SKIPPARSER_HPP)
#define YALR_SKIPPARSER_HPP

#include <boost/spirit/home/x3.hpp>

namespace yalr { namespace parser {
namespace x3 = boost::spirit::x3;

struct skipparser: x3::parser<char> {

    using attribute_type = x3::unused_type;
    
    template<typename Iterator, typename Context, typename RContext, typename Attribute>
    bool parse(Iterator &first, Iterator const& last, Context const&, 
               RContext const&, Attribute&) const
    {

        auto save = first;
        bool in_comment = false;
        while (first != last) {
            std::cerr << "Char = " << *first << "\n";
            if (in_comment) {
                if (*first == '*') {
                    // start of closing marker. check if we have the second character.
                    // If not, just swallow the star.
                    ++first;
                    if ((first != last) && (*first == '/')) {
                        in_comment = false;
                        ++first;
                    } 
                } else {
                    ++first;
                }
            } else {
                if ( *first == '/') {
                    // Start of opening marker. check if we have second character.
                    // If we don't we'll need to backup and let the "real" parser
                    // handle it.
                    auto restore = first;
                    ++first;
                    if ((first != last) && (*first == '*')) {
                        in_comment = true;
                        ++first;
                    } else {
                        first = restore;
                        break;
                    }
                } else if (std::isspace(*first)) {
                    ++first;
                } else {
                    break;
                }
            }
        }

        return (save != first);
	}


};

}}

#endif
