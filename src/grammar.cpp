#include "grammar.hpp"
#include "skipparser.hpp"

namespace x3 = boost::spirit::x3;

namespace yalr { 
    namespace parser {

        using x3::int_;
        using x3::alnum;
        using x3::lit;
        using x3::double_;
        using x3::lexeme;
        using x3::ascii::char_;

        x3::rule<class grammar_tag> const grammar = "grammar";
        x3::rule<class parser_class_tag> const parser_class = "parser_class";

        auto const ualnum = alnum | char_('_');

        x3::symbols<int> symtab;

        auto mkkw = [](std::string kw) { 
            symtab.add(kw);
            return lexeme[x3::lit(kw) >> !ualnum]; 
        };

        auto const kw_prsr = mkkw("parser");
        auto const kw_class = mkkw("class");

        auto const reserved = lexeme[symtab >> !ualnum];

        auto const quoted_string = lexeme['"' >> +(char_ - '"') >> '"'];

        auto const parser_class_def = kw_prsr >> kw_class >> 
                quoted_string  >> ';' ;
        BOOST_SPIRIT_DEFINE(parser_class);

        auto const grammar_def =
            parser_class
            ;

        BOOST_SPIRIT_DEFINE(grammar);

        using grammar_type = x3::rule<class grammar_tag>;
		using context_type = x3::phrase_parse_context<skipparser>::type;

        BOOST_SPIRIT_INSTANTIATE(grammar_type, iterator_type, context_type);
    }

	std::pair<bool, parser::ast_tree_type>do_parse(
			parser::iterator_type &first, parser::iterator_type const &last) {

        x3::unused_type e;
        auto skipper = parser::skipparser();

        bool r = phrase_parse(first, last, parser::grammar_type(),
			 skipper, e);

        return std::make_pair(r, e);
    }
}
