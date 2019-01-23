#if !defined(YALR_TYPE_NAME_HPP)
#define YALR_TYPE_NAME_HPP

#include <boost/spirit/home/x3.hpp>
#include <boost/spirit/home/x3/core/parser.hpp>
#include <boost/spirit/home/x3/core/skip_over.hpp>

#include <cassert>

namespace yalr::parser {

namespace x3 = boost::spirit::x3;

struct type_name : x3::parser<char> {

    using attribute_type = std::string;

    template<typename Iterator, typename Context, typename RContext, typename Attribute>
    bool parse(Iterator &first, Iterator const& last, Context const& context, 
               RContext const&, Attribute& attr) const
    {

        //boost::spirit::x3::traits::move_to(count, attr);
        x3::skip_over(first, last, context);

        auto save = first;
        auto found_it = false;

        auto attr_start = first;
        auto attr_end = last;

        if (first != last) {
            if (*first == '<') {
                int count = 1;
                attr_start += 1;
                while (++first != last and not found_it) {
                    switch (*first) {
                        case '<' :
                            count += 1;
                            break;
                        case '>' :
                            count -= 1;
                            break;
                    }
                    if (count ==0) {
                        attr_end = first;
                        found_it = true;
                        // don't break. let the loop increment the iterator
                    }
                }
            }
        }

        if (found_it) {
            // found it
            //std::cout << "Found it!\n";
            attr = std::string(attr_start, attr_end);
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
