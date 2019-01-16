#if !defined(YALR_QUOTED_PATTERN_HPP)
#define YALR_QUOTED_PATTERN_HPP

#include <boost/spirit/home/x3.hpp>
#include <boost/spirit/home/x3/core/parser.hpp>
#include <boost/spirit/home/x3/core/skip_over.hpp>

#include <cassert>

namespace yalr::parser {

namespace x3 = boost::spirit::x3;

/* Find a regex in double quotes. So, need to ignore escaped quotes
 * An quotes that show up in character classes
 */
struct quoted_pattern: x3::parser<char> {

    using attribute_type = std::string;
    
    enum State { INIT , ESCAPE };

    template<typename Iterator, typename Context, typename RContext, typename Attribute>
    bool parse(Iterator &first, Iterator const& last, Context const& context, 
               RContext const&, Attribute& attr) const
    {

        //boost::spirit::x3::traits::move_to(count, attr);
        x3::skip_over(first, last, context);

        //std::cout << "Entering quoted_pattern. *first = '" << *first << "'\n";

        auto save = first;
        // punt on the above for now. Just run til we see the next quote.

        if ((first != last) and (*first == '"')) {
            //std::cout<< "Saw the first quote\n";
            int state = INIT;
            bool stop = false;
            while (++first != last and not stop) {
                switch (state) {
                    case INIT :
                        if (*first == '\\') {
                            state = ESCAPE;
                        } else if (*first == '"') {
                            stop = true;
                        }
                        break;
                    case ESCAPE :
                        state = INIT;
                        break;
                    default :
                        assert(false);
                }
            }

            if (not stop) {
                //std::cout << "backing up\n";
                // backup
                first = save;
            } else {
                // found it
                //std::cout << "Found it!\n";
                attr = std::string(save, first);
                //std::cout << "attr = " << attr << "\n";
            }
        }

        return ( first != save);
    }
};



} // namespace yalr::parser

#endif
