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
struct quoted_pattern : x3::parser<char> {

    using attribute_type = std::string;

    enum State { INIT , ESCAPE };

    template<typename Iterator>
    bool single_quotes(Iterator& first, Iterator const& last) const {
        int state = INIT;
        bool stop = false;
        // put the increment first. That way we consume the quote
        // that stopped us.
        while (++first != last and not stop) {
            switch (state) {
                case INIT :
                    if (*first == '\\') {
                        state = ESCAPE;
                    } else if (*first == '\'') {
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

        return stop;
    }


    template<typename Iterator>
    bool raw_string(Iterator& first, Iterator const& last) const {
        // the caller made sure that the 'r' was there, so skip over it and
        // check the ':'
        if (*(++first) != ':') {
            return false;
        }
        int state = INIT;
        bool stop = false;
        // put stop first so we don't increment if we are stopping.
        // We do not want the space that forced us to stop.
        while (not stop and ++first != last) {
            switch (state) {
                case INIT :
                    if (*first == '\\') {
                        state = ESCAPE;
                    } else if (std::isspace(*first)) {
                        stop = true;
                    }
                    break;
                case ESCAPE :
                    if (*first == '\n') {
                        stop = true;
                    }
                    state = INIT;
                    break;
                default :
                    assert(false);
            }
        }
        return stop;
    }

    template<typename Iterator, typename Context, typename RContext, typename Attribute>
    bool parse(Iterator &first, Iterator const& last, Context const& context, 
               RContext const&, Attribute& attr) const
    {

        //boost::spirit::x3::traits::move_to(count, attr);
        x3::skip_over(first, last, context);

        //std::cout << "Entering quoted_pattern. *first = '" << *first << "'\n";

        auto save = first;

        auto found_it = false;

        if (first != last) {
            if (*first == '\'') {
                found_it = single_quotes(first, last);
            } else if (*first == 'r') {
                found_it = raw_string(first, last);
            }

            if (found_it) {
                // found it
                //std::cout << "Found it!\n";
                attr = std::string(save, first);
                //std::cout << "attr = " << attr << "\n";
            } else {
                //std::cout << "backing up\n";
                // backup
                first = save;
            }
        }

        return ( first != save);
    }
};


} // namespace yalr::parser

#endif
