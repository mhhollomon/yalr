#if !defined(YALR_SINGLE_QUOTE_HPP)
#define YALR_SINGLE_QUOTE_HPP

#include <boost/spirit/home/x3.hpp>
#include <boost/spirit/home/x3/core/parser.hpp>
#include <boost/spirit/home/x3/core/skip_over.hpp>

#include <cassert>

namespace yalr::parser {

namespace x3 = boost::spirit::x3;

struct single_quote : x3::parser<char> {

    using attribute_type = std::string;

    enum State { INIT , ESCAPE };

    template<typename Iterator, typename Context, typename RContext, typename Attribute>
    bool parse(Iterator &first, Iterator const& last, Context const& context, 
               RContext const&, Attribute& attr) const
    {

        //boost::spirit::x3::traits::move_to(count, attr);
        x3::skip_over(first, last, context);

        auto save = first;
        auto found_it = false;
        auto error = false;

        //std::cout << "Entering quoted_pattern. *first = '" << *first << "'\n";
        int state = INIT;
        if (first != last) {
            if (*first == '\'') {
                while (not error and ++first != last and not found_it) {
                    switch (state) {
                        case INIT :
                            if (*first == '\\') {
                                state = ESCAPE;
                            } else if (*first == '\'') {
                                found_it = true;
                            } else if (*first == '\n') {
                                error = true;
                            }
                            break;
                        case ESCAPE :
                            state = INIT;
                            if (*first == '\n') {
                                error = true;
                            }
                            break;
                        default :
                            assert(false);
                    }
                }
            }
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

        return found_it;
    }
};


} // namespace yalr::parser

#endif
