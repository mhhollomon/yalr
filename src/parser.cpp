#include "parser.hpp"
#include "skipparser.hpp"

#include "quoted_pattern.hpp"

#include <boost/spirit/home/x3/support/utility/error_reporting.hpp>
#include <boost/spirit/home/x3/support/utility/annotate_on_success.hpp>

namespace yalr {
    namespace parser {
        struct error_handler
        {
            template <typename Iterator, typename Exception, typename Context>
            x3::error_handler_result on_error(
                Iterator& /*first*/, Iterator const& /*last*/
              , Exception const& x, Context const& context)
            {
                auto& error_handler = x3::get<x3::error_handler_tag>(context).get();
                std::string message = "Error! Expecting: " + x.which() + " here:";
                error_handler(x.where(), message);
                return x3::error_handler_result::fail;
            }
        };

        struct rule_stmt_tag : x3::annotate_on_success {};
        struct alternative_tag : x3::annotate_on_success {};
        struct term_tag : error_handler, x3::annotate_on_success {};
        struct yskip_tag : error_handler, x3::annotate_on_success {};
        struct grammar_tag : x3::annotate_on_success {};

        using x3::alnum;
        using x3::alpha;
        using x3::lit;
        using x3::lexeme;
        using x3::ascii::char_;

        x3::rule<grammar_tag, ast::grammar> const grammar = "grammar";
        x3::rule<class parser_class_tag, std::string> const parser_class = "parser_class";
        x3::rule<rule_stmt_tag, ast::rule_def> const rule_stmt = "rule_stmt";
        x3::rule<alternative_tag, ast::alternative> const alternative = "alternative";
        x3::rule<term_tag, ast::terminal> const terminal = "terminal";
        x3::rule<yskip_tag, ast::skip> const yskip = "yskip";
        x3::rule<class ident_tag, std::string> const ident = "ident";

        auto const ualnum = alnum | char_('_');

        x3::symbols<int> symtab;

        auto mkkw = [](const std::string & kw) { 
            symtab.add(kw);
            return lexeme[lit(kw) >> !ualnum]; 
        };

        /* reserved keyword */
        auto const kw_prsr = mkkw("parser");
        auto const kw_class = mkkw("class");
        auto const kw_rule = mkkw("rule");
        auto const kw_term = mkkw("term");
        auto const kw_skip = mkkw("skip");
        auto const kw_goal = mkkw("goal");

        auto const reserved = lexeme[symtab >> !ualnum];

        /* Identifiers */
        auto const ident_def = 
            lexeme[ *char_('_') >> alpha >> *ualnum ] - reserved ;

        /* parser class xyz ; */
        auto const parser_class_def = kw_prsr >> kw_class > 
                ident  > ';' ;

        /* term Z "pattern" ; */
        auto const terminal_def = kw_term > ident > -quoted_pattern() > x3::lit(';') ;

        /* skip Z "pattern" ;
         * Note, that a pattern is required for a skip. */
        auto const yskip_def = kw_skip > ident > quoted_pattern() > x3::lit(';') ;

        /* rule X { => A B C ; => X Y Z ; } */
        /* I don't really want to capture the fat arrow, but without it, Spirit
         * tries to hoist the vector of idents into the alternatives struct itself.
         */
        auto const alternative_def = x3::string("=>") > *ident > x3::lit(';') ;

        auto const rule_stmt_def =  ((kw_goal >> x3::attr(true)) | x3::attr(false)) >> kw_rule > ident  >
            x3::lit('{') >> +alternative >> x3::lit('}')
            ;


        /* Top Level Grammar */
        auto const grammar_def =
            (-parser_class >> 
            +( yskip | terminal | rule_stmt )) >
            x3::eoi
            ;

        BOOST_SPIRIT_DEFINE(ident);
        BOOST_SPIRIT_DEFINE(parser_class);
        BOOST_SPIRIT_DEFINE(terminal);
        BOOST_SPIRIT_DEFINE(yskip);
        BOOST_SPIRIT_DEFINE(alternative);
        BOOST_SPIRIT_DEFINE(rule_stmt);
        BOOST_SPIRIT_DEFINE(grammar);

        using grammar_type = x3::rule<grammar_tag, ast::grammar>;
		using phrase_context_type = x3::phrase_parse_context<skipparser>::type;
        using context_type =  x3::context<
            x3::error_handler_tag,
            std::reference_wrapper<x3::error_handler<iterator_type>> const,
            phrase_context_type>
                ;


        BOOST_SPIRIT_INSTANTIATE(grammar_type, iterator_type, context_type);

    } // namespace parser

    namespace x3 = boost::spirit::x3;

	parser::ast_tree_type do_parse(
			parser::iterator_type &first, parser::iterator_type const &last) {

        parser::ast_tree_type e;
        auto skipper = parser::skipparser();

        x3::error_handler<parser::iterator_type> 
            error_handler(first, last, std::cerr);

        auto const parser =
            x3::with<x3::error_handler_tag>(std::ref(error_handler))
                [
                    parser::grammar
                ];


        bool r = phrase_parse(first, last, parser, skipper, e);
        e.success = r;

        return e;
    }

} // namespace yalr
