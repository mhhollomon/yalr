#include "parser.hpp"

#include <cassert>
#include <set>
#include <list>
#include <memory>

namespace yalr {

using namespace std::literals::string_literals;
using namespace std::literals::string_view_literals;

struct parse_loc {
    std::string_view sv;
    text_offset_t offset;
};


std::set<std::string, std::less<>> keywords = {
    "parser", "class", "rule", "term", "skip", "global",
    "namespace", "lexer", "option"
};

struct parser_guts {

    std::shared_ptr<text_source> source;

    // The string_view here has the already parsed text
    // removed. current_loc.sv is what is left to be parsed.
    // current_loc.offset, however, is the offset inside
    // orig_sv (above). It should always be the case that
    // current_loc.sv = orig_sv.remove_prefix(current_loc.offset-1)
    //
    parse_loc current_loc;

    //
    // Errors that have been encountered so far.
    //
    std::list<error_info> error_list;

    //
    // Constructor
    //
    parser_guts(std::shared_ptr<text_source> _source) : 
        source{_source}, current_loc{_source->content, 0} {
    }

    /****************************************************************
     * Error Handling
     ****************************************************************/

    //
    // Add an error at the current location
    //
    void record_error(const std::string& msg) {
        record_error(msg, current_loc);
    }

    //
    // Add an error at the given location
    //
    void record_error(const std::string& msg, parse_loc& loc) {
        error_list.emplace_back(msg, 
                text_fragment{std::string_view(), pl_to_tl(loc)});
    }

    //
    // Output the errors to the given ostream.
    //
    std::ostream& stream_errors(std::ostream& ostrm) const {
        for (const auto& iter : error_list) {
            iter.output(ostrm);
        }

        return ostrm;
    }

    
    /****************************************************************
     * Input Utilities
     ****************************************************************/
    //
    // check if the current location is at the end of the input
    //
    inline bool eoi() { return current_loc.sv.size() == 0; }

    //
    // Return the next character in the stream. If we're at the 
    // end of the input then return the "special character" NUL
    //
    inline char peek() { if (!eoi()) { return current_loc.sv[0]; } 
            else return '\x00'; }

    //
    // Return the character `pos` characters ahead of the
    // current one.
    // DANGER - this isn't checked. use valid_pos() first
    // 
    inline char peek(int pos) { return current_loc.sv[pos]; }

    //
    // Check if there is at lepgt pos characters left in the input.
    //
    inline bool valid_pos(int pos) { 
        return (pos >= 0 and (unsigned)pos < current_loc.sv.size()); }

    //
    // Check if the current input starts with the given character sequence.
    // When C++20 comes out this devolves to a call to string_view::starts_with.
    // Does not consume.
    //
    inline bool check_string(std::string_view o) {
        return current_loc.sv.compare(0, o.size(), o) == 0;
    }

    //
    // Remove `count` characters from the input.
    //
    void consume(int count) {
        auto &cl = current_loc;
        cl.offset += count;
        cl.sv.remove_prefix(count);
    }

    /****************************************************************
     * Location utilities
     ****************************************************************/

    //
    // return a text_location from a parse_loc
    // 
    text_location pl_to_tl(parse_loc pl) {
        return {pl.offset, source };
    }

    //
    // return a text_location base on the current_loc
    //
    text_location currl_to_tl() {
        return pl_to_tl(current_loc);
    }

    /****************************************************************
     * Text Fragment utilities
     ****************************************************************/

    text_fragment pl_to_tf(parse_loc pl, int count) {
        return { pl.sv.substr(0, count), pl_to_tl(pl) };
    }

    text_fragment current_loc_text_fragment(int ret_len, int consume_len) {
        auto cl = current_loc;
        consume(consume_len);

        return pl_to_tf(cl, ret_len);
    }

    text_fragment current_loc_text_fragment(int len) {
        return current_loc_text_fragment(len, len);
    }


    /****************************************************************
     * PARSING ROUTINES
     *
     * The naming of these routines follow a pattern:
     * - parse_X - try to parse the nonterminal 'X' - consume and return a parse
     *   tree if successful. Otherwise, consume nothing a return false.
     *
     * - match_X - try to recognize terminal X. Consume and return true (and
     *    possibly other info, if successful. Otherwise consume nothing and
     *    return false.
     *
     * - expect_X - Like match but record and error if unsuccesful.
     ****************************************************************/
    //
    // Top Level parser
    // Entry point
    //
    parse_tree& parse() {

        auto & retval = *(new parse_tree());

        while (skip() and not eoi() and error_list.size() < 5) {
            if (parse_parser_class(retval.statements)) continue;
            if (parse_lexer_class(retval.statements))  continue;
            if (parse_namespace(retval.statements))    continue;
            break;
        }

        while (skip() and not eoi() and error_list.size() < 5) {
            if (parse_rule(retval.statements)) continue;
            if (parse_term(retval.statements)) continue;
            if (parse_skip(retval.statements)) continue;
            record_error("Expecting a statement (rule, term, skip)");
            break;
        }

        retval.success = (eoi() and error_list.size() == 0);

        return retval; 
    }

    //-----------------------------------------------------
    // rule '<' type '>' X { => A B C ; => X Y z:Z <%{ ... }%> } 
    //-----------------------------------------------------
    //
    bool parse_rule(statement_list& stmts) {
        rule new_rule;

        new_rule.isgoal = match_keyword("goal");
        skip();

        if (not match_keyword("rule")) {
            if (new_rule.isgoal) {
                record_error("Expecting 'rule' after 'goal'");
            }
            return false;
        }

        skip();
        new_rule.type_str = match_typestring();

        skip();
        auto name = expect_identifier();
        if (not name) {
           record_error("Expecting an identifier as the name of the rule");
           return false;
        }
        new_rule.name = *name;

        skip();
        if (not expect_char('{')) {
            record_error("Expecting open '{' for list of rule alternatives");
            return false;
        }

        while (skip() and not eoi() and error_list.size() < 5) {
            if (match_char('}')) break;
            if (not parse_alternative(new_rule.alternatives)) {
                record_error("Expecting alternative or rule closing '}'");
                return false;
            }
        }

        stmts.emplace_back(std::move(new_rule));
        return true;
    }

    //-----------------------------------------------------
    // Alternative part of a rule
    //-----------------------------------------------------
    bool parse_alternative(std::vector<alternative>& alts) {
        alternative new_alt;

        if (not check_string("=>")) {
            return false;
        }

        consume(2);

        bool error = false;
        while(skip() and not eoi() and error_list.size() < 5) {
            // ';" closes the alternative, so leave the loop if we see it.
            if (match_char(';')) break;

            // action closes the alternaitve, so leave the loop if we see it.
            auto action_str = match_actionblock();
            if (action_str) {
                new_alt.action = action_str;
                break;
            }

            auto new_item = match_item();
            if (new_item) {
                new_alt.items.emplace_back(*new_item);
                continue;
            }

            // if none of the above, get grumpy.
            record_error("Expecting rule item or action or closing semicolon.");
            error = true;
            break;
        }

        alts.emplace_back(std::move(new_alt));

        return ! error;
    }

    //
    // Match an item in a rule aternative
    //
    // Matches full expression:
    //    (alias ':' )? termName
    //    (alias ':' )? singleQuote
    //
    //    The four possibilities are:
    //    ident
    //    ident : ident
    //    ident : singleQuote
    //    singleQuote
    //
    std::optional<alt_item> match_item() {
        alt_item new_item;
        auto ident = match_identifier();
        if (ident) {
            if (match_char(':')) {
                auto simp  = match_simple_item();
                if (simp) {
                    new_item.symbol_ref = *simp;
                    new_item.alias = ident;
                    return new_item;
                } else {
                    // saw the colon, but not the next piece
                    record_error("Expecting identifier or single quote string");
                    return std::nullopt;
                }
            } else {
                new_item.symbol_ref = *ident;
                return new_item;
            }
        } else {
            auto sq =  match_singlequote();
            if (sq) {
                new_item.symbol_ref = *sq;
                return new_item;
            }
        }
        return std::nullopt;
    }

    //
    // Matches a "simple" item in a rule alternative.
    // Either a single quoted string or an identifier.
    //
    optional_text_fragment match_simple_item() {
        auto otf = match_identifier();
        if (otf) return otf;

        otf =  match_singlequote();
        if (otf) return otf;

        return std::nullopt;
    }

    /* term '<' type '>'  Z pattern ; */
    /* term '<' type '>'  Z pattern <%{ action }%> */
    bool parse_term(statement_list& stmts) {
        terminal new_term;
        optional_text_fragment otf;

        if (not match_keyword("term")) {
            return false;
        }

        skip();
        new_term.type_str = match_typestring();

        skip();
        otf = expect_identifier();
        if (not otf) return false;
        new_term.name = *otf;
        

        skip();
        otf = expect_pattern();
        if (not otf) return false;
        new_term.pattern = *otf;

        skip();
        new_term.action = match_actionblock();
        if (not new_term.action) {
            // if we don't see an action block, we better see the closing semi-colon.
            if (not expect_char(';')) {
                return false;
            }
        }

        stmts.emplace_back(std::move(new_term));

        return true;
    }


    /* skip Z "'" pattern "'" ';'
     * skip Z 'r:' pattern ';'
     * A skip cannot have a type */
    bool parse_skip(statement_list& stmts) {
        yalr::skip new_skip;
        optional_text_fragment otf;

        if (not match_keyword("skip")) {
            return false;
        }

        skip();
        otf = expect_identifier();
        if (not otf) return false;
        new_skip.name = *otf;

        skip();
        otf = expect_pattern();
        if (not otf) return false;
        new_skip.pattern = *otf;

        skip();
        if (not expect_char(';')) return false;

        stmts.emplace_back(std::move(new_skip));

        return true;
    }

    // 
    // 'parser' 'class' IDENT ';'
    //
    bool parse_parser_class(statement_list& stmts) {
        option new_opt;
        optional_text_fragment otf;

        if (not match_keyword("parser")) {
            return false;
        }

        skip();
        if (not expect_keyword("class")) {
            return false;
        }

        skip();
        otf = expect_identifier();
        if (not otf) return false;
        new_opt.setting = *otf;

        skip();
        if (not expect_char(';')) return false;

        new_opt.name = { "parser.class"sv, new_opt.setting.location };

        stmts.emplace_back(std::move(new_opt));

        return true;
    }
    
    // 
    // 'lexer' 'class' IDENT ';'
    //
    bool parse_lexer_class(statement_list& stmts) {
        option new_opt;
        optional_text_fragment otf;

        if (not match_keyword("lexer")) {
            return false;
        }

        skip();
        if (not expect_keyword("class")) {
            return false;
        }

        skip();
        otf = expect_identifier();
        if (not otf) return false;
        new_opt.setting = *otf;

        skip();
        if (not expect_char(';')) return false;


        new_opt.name = { "lexer.class"sv, new_opt.setting.location };
        stmts.emplace_back(std::move(new_opt));

        return true;
    }

    //
    // 'namespace' IDENT ';'
    //
    bool parse_namespace(statement_list& stmts) {
        option new_opt;
        optional_text_fragment otf;

        if (not match_keyword("namespace")) {
            return false;
        }

        skip();
        otf = expect_identifier();
        if (not otf) return false;
        new_opt.setting = *otf;

        skip();
        if (not expect_char(';')) return false;

        new_opt.name = { "namespace"sv, new_opt.setting.location };
        stmts.emplace_back(std::move(new_opt));

        return true;
    }


    /******************************************************
     * PATTERN Matching
     *****************************************************/
    optional_text_fragment expect_pattern() {

        auto lexeme = match_singlequote();
        if (lexeme) {
            return lexeme;
        }

        lexeme = match_regex();
        if (lexeme) {
            return lexeme;
        }

        record_error("Expected pattern");

        return std::nullopt;
    }


    /******************************************************
     * KEYWORD Matching
     *****************************************************/
    // Check if the current location starts with the given keyword.
    // Check that no "interesting" characters follow.
    // If it matches - consume the keyword
    bool match_keyword(std::string_view kw) {
        size_t len = kw.size();

        auto& cl = current_loc;
        auto start_loc = current_loc;
        if (cl.sv.compare(0, len, kw) == 0) {
            consume(len);
            if (eoi() or std::isspace(cl.sv[0])) {
                return true;
            } else {
                auto x = cl.sv[0];
                if (std::isalnum(x) or x == '_') {
                    current_loc = start_loc;
                    return false;
                } else {
                    return true;
                }
            }
        } else {
            return false;
        }
    }

    bool expect_keyword(std::string_view kw) {
        if (match_keyword(kw)) {
            return true;
        } else {
            record_error(
                std::string{"Expecting keyword '"} + 
                std::string{kw} +
                std::string{"'"}
            );
            return false;
        }
    }

    /******************************************************
     * IDENTIFIER Matching
     *****************************************************/
    // Match an identifier (but not a keyword)
    optional_text_fragment match_identifier() {
        auto& cl = current_loc;
        int count = 0;
        while ((unsigned)count < cl.sv.size() and ( 
                    std::isalnum(cl.sv[count]) or cl.sv[count] == '_')) {
            ++count;
        }

        if (count == 0) {
            return std::nullopt;
        } else {
            auto ret_sv = cl.sv.substr(0, count);

            if (keywords.count(ret_sv) > 0) {
                return std::nullopt;
            }

            consume(count);
            return text_fragment{ ret_sv, pl_to_tl(cl) };
        }

    }

    optional_text_fragment expect_identifier() {
        auto retval = match_identifier();

        if (not retval) {
            record_error("Expecting identifier");
        }
        return retval;
    }

    /******************************************************
     * CHAR Matching
     *****************************************************/
    bool match_char(char e) {
        if (peek() == e) {
            consume(1);
            return true;
        } else {
            return false;
        }
    }

    bool expect_char(char e) {
        if (match_char(e)) {
            return true;
        } else {
            record_error(
                    "Expecting '"s +
                    std::string{e} +
                    "'"s
            );
            return false;
        }
    }

    /******************************************************
     * SINGLE QUOTE Matching
     *****************************************************/
    // Match string surrounded by single quotes.
    // newline characters are not allowed unless they are escaped.
    optional_text_fragment match_singlequote() {
        int state = 0;
        if (peek() != '\'') {
            return std::nullopt;
        }
        int count = 1;
        bool working = true;
        bool error = false;
        // put the increment first, that way we move past
        // the character that stopped us. Below, we will consume it,
        // but not return it.
        while(valid_pos(++count) and working) {
            switch (state) {
                case 0: // INIT
                    if (peek(count) == '\\') {
                        state = 1;
                    } else if (peek(count) == '\'') {
                        working = false;
                    } else if (peek(count) == '\n') {
                        record_error("Unescaped new line in single quoted string");
                        error = true;
                        working = false;
                    }
                    break;
                case 1: // ESCAPE
                    state = 0;
                    break;
                default:
                    assert(false);
            }
        }

        if (working) {
            // we ran out of input
            record_error("unterminated single quoted string at end of input");
            error = true;
        }

        if (error) {
            return std::nullopt;
        } else {
            return current_loc_text_fragment(count);
        }
    
    };

    /******************************************************
     * REGEX Matching
     *****************************************************/
    optional_text_fragment match_regex() {
        if (not check_string("r:")) {
            return std::nullopt;
        }
        int state = 0;
        int stop = false;
        int count = 1;
        // check our flag first so we don't advance
        // if we are stopping. We don't want to consume the character
        // that made us stop.
        while (not stop and valid_pos(++count)) {
            switch (state) {
                case 0 :
                    if (peek(count) == '\\') {
                        state = 1;
                    } else if (std::isspace(peek(count))) {
                        stop = true;
                    }
                    break;
                case 1:
                    if (peek(count) == '\n') {
                        stop = true;
                    }
                    state = 0;
                    break;
                default:
                    assert(false);
            }
        }

        return current_loc_text_fragment(count);
    }


    /******************************************************
     * TYPE Match
     *****************************************************/
    optional_text_fragment match_typestring() {
        if (not match_char('<')) {
            return std::nullopt;
        }

        int count = 0;
        bool working = true;
        int braces = 1;
        // put the increment first, that way we move past
        // the character that stopped us. Below, we will consume it,
        // but not return it.
        while(valid_pos(++count) and working) {
            switch (peek(count)) {
                case '<' :
                    braces += 1;
                    break;
                case '>' :
                    braces -= 1;
                    break;
            }

            if (braces <= 0) working = false;
        }

        if (working) {
            // we ran out of input
            record_error("unterminated type string");
        }

        return current_loc_text_fragment(count-1, count);
    }         

    /******************************************************
     * ACTION BLOCK Matching
     *****************************************************/
    optional_text_fragment match_actionblock() {
        if (not check_string("<%{")) {
            return std::nullopt;
        }
        consume(3);
        int state = 0;
        int stop = false;
        int count = -1;
        while (valid_pos(++count) and not stop) {
            switch (state) {
                case 0 :
                    if (peek(count) == '}') state = 1;
                    break;
                case 1 :
                    if (peek(count) == '%') {
                        state = 2;
                    }  else if (peek(count) == '}') {
                        state = 1;
                    } else {
                        state = 0;
                    }
                    break;
                case 2:
                    if (peek(count) == '>') {
                        stop = true;
                    }  else if (peek(count) == '}') {
                        state = 1;
                    } else {
                        state = 0;
                    }
                    break;
            }
        }

        if (not stop) {
            record_error("Unterminated action block");
            return std::nullopt;
        }

        return current_loc_text_fragment(count-3, count);

    }
    /****************************************************************
     * SKIP Processing
     ****************************************************************/
    bool skip() {

        //
        // States for the state machine
        //
        enum SKIP_STATE { skNormal, skLine, skBlock };

        SKIP_STATE state = skNormal;

        //
        // we'll be using this a lot.
        // So abbreviate it with a reference.
        //
        auto &cl = current_loc;

        //
        // When we enter a comment, we'll record where it started.
        // If, at the end, we're still looking for comment terminator,
        // we can use this location to put out an error message.
        //
        parse_loc comment_loc;

        bool skipping = true;
        while (skipping and not eoi()) {

            //std::cout << "Loop top pos = " << cl.offset << 
            //    " state = " << state <<
            //    " char = '" << peek() << "'\n";
            switch (state) {
                case skNormal : // Normal state
                    if (std::isspace(peek(0))) {
                        consume(1);
                    } else if (check_string("//")) {
                        //std::cout << "--- Saw line comment start\n";
                        comment_loc = cl;
                        consume(2);
                        state = skLine;
                    } else if (check_string("/*")) {
                        //std::cout << "--- Saw block comment start\n";
                        comment_loc = cl;
                        consume(2);
                        state = skBlock;
                    } else {
                        skipping = false;
                    }
                    break;
                case skLine: // Line comment
                    if (peek(0) == '\n') {
                        //std::cout << "--- line comment end\n";
                        state = skNormal;
                    }
                    consume(1);
                    break;
                case skBlock: // Block comment
                    if (check_string("*/")) {
                        //std::cout << "--- block comment end\n";
                        consume(2);
                        state = skNormal;
                    } else {
                        consume(1);
                    }
                    break;
            }
        }

        if (state != skNormal) {
            record_error("Unterminated comment starting here", comment_loc);
            return false;
        }

        return true;
    }

/*** End of parser_guts ***/
};


//
// yalr_parser stuff
//
yalr_parser::yalr_parser(std::shared_ptr<yalr::text_source> src) :
    guts{std::make_unique<parser_guts>(src)}
{}

//
// default back the destructor. This little game is
// just to force the compiler to generate the destructor
// here (late) after parser_guts is fully defined.
// c.f. e.g. https://www.fluentcpp.com/2017/09/22/make-pimpl-using-unique_ptr/
//
yalr_parser::~yalr_parser() = default;

std::ostream& yalr_parser::stream_errors(std::ostream& ostrm) {
    return guts->stream_errors(ostrm); 
}

parse_tree& yalr_parser::parse() {
    return guts->parse();
}

/***************************************/
} // namespace
