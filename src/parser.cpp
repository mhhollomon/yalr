#include "parser.hpp"
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

        x3::rule<class grammar_tag, ast::grammar> const grammar = "grammar";
        x3::rule<class parser_class_tag, std::string> const parser_class = "parser_class";
        x3::rule<class rule_stmt_tag, ast::rule_def> const rule_stmt = "rule_stmt";
        x3::rule<class alternative_tag, ast::alternative> const alternative = "alternative";
        x3::rule<class term_tag, ast::terminal> const terminal = "terminal";
        x3::rule<class ident_tag, std::string> const ident = "ident";

        auto const ualnum = alnum | char_('_');

        x3::symbols<int> symtab;

        auto mkkw = [](std::string kw) { 
            symtab.add(kw);
            return lexeme[x3::lit(kw) >> !ualnum]; 
        };

        auto const kw_prsr = mkkw("parser");
        auto const kw_class = mkkw("class");
        auto const kw_rule = mkkw("rule");
        auto const kw_term = mkkw("term");

        auto const reserved = lexeme[symtab >> !ualnum];

        auto const ident_def = 
            lexeme[ *char_('_') >> x3::alpha >> *ualnum ] - reserved ;

        /* parser class "xyz" ; */
        auto const parser_class_def = kw_prsr >> kw_class >> 
                ident  >> ';' ;

        /* term Z ; */
        auto const terminal_def = kw_term >> x3::attr(3) >> ident >> x3::lit(';') ;

        /* rule X { => A B C ; => X Y Z ; } */
        /* I don't really want to capture the fat arrow, but without it, Spirit
         * tries to hoist the vector of idents into the alternatives struct itself.
         */
        auto const alternative_def = x3::string("=>") >> +ident >> x3::lit(';') ;

        auto const rule_stmt_def = kw_rule >> ident  >>
            x3::lit('{') >> +alternative >> x3::lit('}')
            ;


        /* Top Level Grammar */
        auto const grammar_def =
            -parser_class >> 
            +( terminal | rule_stmt )
            ;

        BOOST_SPIRIT_DEFINE(ident);
        BOOST_SPIRIT_DEFINE(parser_class);
        BOOST_SPIRIT_DEFINE(terminal);
        BOOST_SPIRIT_DEFINE(alternative);
        BOOST_SPIRIT_DEFINE(rule_stmt);
        BOOST_SPIRIT_DEFINE(grammar);

        using grammar_type = x3::rule<class grammar_tag, ast::grammar>;
		using context_type = x3::phrase_parse_context<skipparser>::type;

        BOOST_SPIRIT_INSTANTIATE(grammar_type, iterator_type, context_type);
    }

	std::pair<bool, parser::ast_tree_type>do_parse(
			parser::iterator_type &first, parser::iterator_type const &last) {

        parser::ast_tree_type e;
        auto skipper = parser::skipparser();

        bool r = phrase_parse(first, last, parser::grammar_type(),
			 skipper, e);

        return std::make_pair(r, e);
    }
}
