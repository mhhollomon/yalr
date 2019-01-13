#if !defined(YALR_SKIPPARSER_HPP)
#define YALR_SKIPPARSER_HPP

#include <boost/spirit/home/x3.hpp>

namespace yalr::parser {
namespace x3 = boost::spirit::x3;

struct skipparser: x3::parser<char> {

    using attribute_type = x3::unused_type;
    
    enum { INIT, COMMENT_B1, BLOCK, BLOCK_E1, LINE };
    template<typename Iterator, typename Context, typename RContext, typename Attribute>
    bool parse(Iterator &first, Iterator const& last, Context const&, 
               RContext const&, Attribute&) const
    {

        auto orig = first;
        auto save = first;

        int state = INIT;
        bool stop = false;

        while (first != last and not stop) {
            switch (state) {
                case INIT :
                    if (*first == '/') {
                        save = first;
                        state = COMMENT_B1;
                        ++first;
                    } else if (std::isspace(*first)) {
                        ++first;
                    } else {
                        stop = true;
                    }
                    break;
                case COMMENT_B1:
                    if (*first == '*') {
                        state = BLOCK;
                        ++first;
                    } else if (*first == '/') {
                        state = LINE;
                        ++first;
                    } else {
                        first = save;
                        stop = true;
                        // don't advance. let the '/' be handled
                        // by somebody else.
                        // Make the state INIT so we can see errors.
                        // state = INIT;
                    }
                    break;
                case BLOCK :
                    if (*first == '*') {
                        state = BLOCK_E1;
                    }
                    ++first;
                    break;
                case BLOCK_E1 :
                    if (*first == '*') {
                        state = BLOCK_E1;
                    } else if (*first == '/') {
                        state = INIT;
                    } else {
                        state = BLOCK;
                    }
                    ++first;
                    break;
                case LINE :
                    if (*first == '\n') {
                        state = INIT;
                    }
                    ++first;
                    break;
            };
        }
        return (orig != first);
	}


};

} // namespace yalr::parser

#endif
