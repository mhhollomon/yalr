#if !defined(YALR_QUOTED_PATTERN_HPP)
#define YALR_QUOTED_PATTERN_HPP

#include <boost/spirit/home/x3.hpp>
#include <boost/spirit/home/x3/core/parser.hpp>
#include <boost/spirit/home/x3/core/skip_over.hpp>

#include <regex>
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
    bool double_quotes(Iterator& first, Iterator const& last) const {
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

        return stop;
    }

    std::regex raw_regex;

    quoted_pattern() : raw_regex{R"%(R"([^\\ \t\n]{0,16})((.|\n)(?!\)\1))*(.|\n)?\)\1")%", 
        std::regex_constants::optimize}  {}
    
    template<typename Iterator>
    bool raw_string(Iterator& first, Iterator const& last) const {
        std::match_results<std::string::const_iterator> m;
        bool r = std::regex_search(first, last, m, raw_regex, std::regex_constants::match_continuous);
        if (r) {
            first += m.length(0);
        }
        return r;
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
            if (*first == '"') {
                found_it = double_quotes(first, last);
            } else if (*first == 'R') {
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
