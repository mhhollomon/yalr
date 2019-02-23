#if not defined(YALR_PARSER_HPP)
#define YALR_PARSER_HPP

#include "ast.hpp"

#include <stack>
#include <list>
#include <vector>
#include <set>
#include <ctype.h>

#include <cassert>

namespace yalr::parser {

    using match_val = std::pair<bool, std::string_view>;

    std::set<std::string, std::less<>> keywords = {
        "parser", "class", "rule", "term", "skip", "global",
        "namespace", "lexer", "option"
    };

    struct parse_loc {
        std::string_view sv;
        long offset;
    };

    struct error_record {
        std::string message;
        parse_loc pl;

        error_record(std::string m, parse_loc x) : message{m}, pl{x} {}
    };

    struct line_start {
        long line_num;
        long offset;
        line_start(long l, long o) : line_num(l), offset(o) {}
    };

    struct yalr_parser {
        std::string_view orig_sv;
        parse_loc current_loc;

        std::list<error_record> error_list;

        std::vector<line_start> line_map;
        long current_line = 1;

        yalr_parser(std::string_view sv) : 
            orig_sv{sv}, current_loc{sv, 0} {
            
            line_map.reserve(1000);
            line_map.emplace_back(1, 0);
        }

        line_start find_line(long offset) {
            std::vector<line_start>::size_type start = 0;
            std::vector<line_start>::size_type end = line_map.size();
            auto index = start + (end - start)/2;
            while(start < end-1) {
                //std::cerr << "s=" << start << " i=" << index << " e=" << end << '\n';
                // exact match - return it.
                if (line_map[index].offset == offset) return line_map[index];

                if (line_map[index].offset > offset) {
                    // we passed it. So everything north is also out of scope.
                    // reduce our range to the items before this.
                    if (index == end) {
                        end -= 1;
                    } else {
                        end = index;
                    }
                } else {
                    // Less than. Need to move forward more
                    // Note that this keeps `index` as a part of the range.
                    if (index == start) {
                        index += 1;
                        if (index == end) break;
                        continue;
                    }
                    start = index;
                }
                index = start + (end - start)/2;
            }

            // ran out of places to look. Return whatever we have.
            return line_map[start];
        }


        std::ostream& stream_errors(std::ostream& ostrm) {
            for (const auto& iter : error_list) {
                long offset = iter.pl.offset;
                auto [line_num, line_start] = find_line(offset);
                auto column = offset-line_start + 1;

                ostrm << "filename " << line_num << ':' <<
                    column << " (offset = " << offset << ")" << "  : " <<
                    iter.message << "\n";
                auto sv = orig_sv;
                sv.remove_prefix(line_start);
                auto pos = sv.find_first_of('\n');
                if (pos != std::string_view::npos) {
                    sv = sv.substr(0, pos);
                }

                ostrm << sv;
                if (pos == std::string_view::npos) {
                    ostrm << "\n";
                }
                std::string filler(column, '~');
                ostrm << filler << "^\n";
            }

            return ostrm;
        }

        inline bool eoi() { return current_loc.sv.size() == 0; }
        inline char peek() { if (!eoi()) { return current_loc.sv[0]; } else return '\x00'; }
        // DANGER - this isn't checked. use valid_pos() first
        inline char peek(int pos) { return current_loc.sv[pos]; }
        inline bool valid_pos(int pos) { return (pos >= 0 and pos < current_loc.sv.size()); }
        void consume(int count) {
            auto &cl = current_loc;
            while (count > 0 and not eoi() ) {
                cl.offset += 1;
                if (cl.sv[0] == '\n') {
                    current_line += 1;
                    line_map.emplace_back(current_line, cl.offset);
                }
                cl.sv.remove_prefix(1);
                count -= 1;
            }
        }
        // only use if you are sure the consumed text cannot contain a 
        // new line.
        void fast_consume(int count) {
            auto &cl = current_loc;
            cl.offset += count;
            cl.sv.remove_prefix(count);
        }

        void record_error(const std::string& msg) {
            error_list.emplace_back(msg, current_loc);
        }

        void record_error(const std::string& msg, parse_loc& loc) {
            error_list.emplace_back(msg, loc);
        }

        // Top Level parser
        // Entry point
        ast::grammar parse() {

            ast::grammar retval;

            skip();
            while (skip() and not eoi() and error_list.size() < 5) {
                if (parse_parser_class(retval)) continue;
                if (parse_lexer_class(retval))  continue;
                if (parse_namespace(retval))    continue;
                break;
            }

            while (skip() and not eoi() and error_list.size() < 5) {
                if (parse_rule(retval.defs)) continue;
                if (parse_term(retval.defs)) continue;
                if (parse_skip(retval.defs)) continue;
                record_error("Expecting a statement (rule, term, skip)");
                break;
            }

            retval.success = (eoi() and error_list.size() == 0);

            return retval; 
        }

        /* rule '<' type '>' X { => A B C ; => X Y Z ; } */
        bool parse_rule(ast::def_list_type& defs) {
            ast::rule_def new_sym;

            new_sym.offset = current_loc.offset;

            if (match_keyword("goal")) {
                new_sym.isgoal = true;
                skip();
            } else {
                new_sym.isgoal = false;
            }

            if (not match_keyword("rule")) {
                if (new_sym.isgoal) {
                    record_error("expecting rule after 'goal'");
                }
                return false;
            }

            skip();
            auto [saw_type, type_string] = match_typestring();
            if (saw_type) {
                new_sym.type_str = std::string(type_string);
                skip();
            }

            auto [success, identifier] = expect_identifier();
            if (not success) return false;

            if (not skip() or not expect_char('{')) return false;

            while (skip() and not eoi() and error_list.size() < 5) {
                if (match_char('}')) break;
                if (not parse_alternative(new_sym.alts)) break;
            }

            new_sym.name = std::string(identifier);
            defs.emplace_back(std::move(new_sym));
            return true;
        }

        bool parse_alternative(std::vector<ast::alternative>& alts) {
            ast::alternative new_alt;

            new_alt.offset = current_loc.offset;

            if (not check_string("=>")) {
                return false;
            }

            fast_consume(2);

            bool error = false;
            while(skip() and not eoi() and error_list.size() < 5) {
                if (match_char(';')) break;
                auto [saw_id, ident] = match_identifier();
                if (saw_id) {
                    new_alt.pieces.emplace_back(ident);
                    continue;
                }
                auto [saw_sq, sq] =  match_singlequote();
                if (saw_sq) {
                    new_alt.pieces.emplace_back(sq);
                    continue;
                }
                record_error("Expecting identifier or single quote string");
                error = true;
                break;
            }

            alts.emplace_back(std::move(new_alt));

            return ! error;
        }

        /* term '<' type '>'  Z pattern ; */
        /* term '<' type '>'  Z pattern <%{ action }%> */
        bool parse_term(ast::def_list_type& defs) {
            ast::terminal new_sym;

            new_sym.offset = current_loc.offset;

            if (not match_keyword("term")) {
                return false;
            }

            skip();
            auto [saw_type, type_string] = match_typestring();
            if (saw_type) {
                new_sym.type_str = std::string(type_string);
                skip();
            }

            auto [saw_id, identifier] = expect_identifier();
            if (not saw_id) return false;
            new_sym.name = std::string(identifier);

            skip();
            auto [saw_patt, pattern] = expect_pattern();
            if (not saw_patt) return false;
            new_sym.pattern = std::string(pattern);

            skip();
            auto [ saw_act, action_str ] = match_actionblock();
            if (saw_act) {
                new_sym.action = std::string(action_str);
            } else if (not expect_char(';')) {
                return false;
            }

            defs.emplace_back(std::move(new_sym));

            return true;
        }


        /* skip Z "'" pattern "'" ';'
         * skip Z 'r:' pattern ';'
         * A skip cannot have a type */
        bool parse_skip(ast::def_list_type& defs) {
            ast::skip new_sym;

            new_sym.offset = current_loc.offset;

            if (not match_keyword("skip")) {
                return false;
            }

            skip();
            auto [success, identifier] = expect_identifier();
            if (not success) return false;
            new_sym.name = std::string(identifier);

            skip();
            auto [saw_pat, pattern] = expect_pattern();
            if (not saw_pat) return false;
            new_sym.pattern = std::string(pattern);

            skip();
            if (not expect_char(';')) return false;

            defs.emplace_back(std::move(new_sym));

            return true;
        }

        // 
        // 'parser' 'class' IDENT ';'
        //
        bool parse_parser_class(ast::grammar& g) {

            auto start_loc = current_loc;

            if (not match_keyword("parser")) {
                return false;
            }

            skip();
            if (not expect_keyword("class")) {
                return false;
            }

            if (not g.parser_class.empty()) {
                record_error("Duplicate parser class declaration", start_loc);
                return false;
            }

            skip();
            auto [success, identifier] = expect_identifier();

            if (not success) return false;

            skip();
            if (not expect_char(';')) return false;

            g.parser_class = identifier;
            return true;
        }
        
        // 
        // 'lexer' 'class' IDENT ';'
        //
        bool parse_lexer_class(ast::grammar& g) {

            auto start_loc = current_loc;

            if (not match_keyword("lexer")) {
                return false;
            }

            skip();
            if (not expect_keyword("class")) {
                return false;
            }

            if (not g.lexer_class.empty()) {
                record_error("Duplicate lexer class declaration", start_loc);
                return false;
            }

            skip();
            auto [success, identifier] = expect_identifier();

            if (not success) return false;

            skip();
            if (not expect_char(';')) return false;

            g.lexer_class = identifier;

            return true;
        }

        //
        // 'namespace' IDENT ';'
        //
        bool parse_namespace(ast::grammar& g) {

            auto start_loc = current_loc;

            if (not match_keyword("namespace")) {
                return false;
            }

            if (not g.code_namespace.empty()) {
                record_error("Duplicate namespace declaration", start_loc);
                return false;
            }

            skip();
            auto [success, identifier] = expect_identifier();

            if (not success) return false;

            skip();
            if (not expect_char(';')) return false;

            g.code_namespace = identifier;

            return true;
        }


        match_val expect_pattern() {

            {
                auto [success, lexeme] = match_singlequote();
                if (success) {
                    return {true, std::move(lexeme)};
                }
            }

            {
                auto [success, lexeme] = match_regex();
                if (success) {
                    return {true, std::move(lexeme)};
                }
            }

            record_error("Expected pattern");

            return {false, ""};
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
                fast_consume(len);
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
        match_val match_identifier() {
            auto& cl = current_loc;
            int count = 0;
            while (count < cl.sv.size() and ( 
                        std::isalnum(cl.sv[count]) or cl.sv[count] == '_')) {
                ++count;
            }

            if (count == 0) {
                return { false, std::string_view{""} };
            } else {
                auto ret_sv = cl.sv.substr(0, count);

                if (keywords.count(ret_sv) > 0) {
                    return { false, std::string_view{""} };
                }

                fast_consume(count);
                return { true, ret_sv };
            }

        }

        match_val expect_identifier() {
            auto retval = match_identifier();

            if (not retval.first) {
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
                        std::string{"Expecting '"} +
                        std::string{e} +
                        std::string{"'"}
                );
                return false;
            }
        }

        /******************************************************
         * SINGLE QUOTE Matching
         *****************************************************/
        // Match string surrounded by single quotes.
        // newline characters are not allowed unless they are escaped.
        match_val match_singlequote() {
            int state = 0;
            if (peek() != '\'') {
                return {false, ""};
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
                record_error("unterminated singe quoted string at end of input");
            }

            auto sv = current_loc.sv.substr(0, count);
            // need to use consume since there may be embedded 
            // (but escaped) newlines.
            consume(count);

            return {true, std::move(sv)};
        
        };

        /******************************************************
         * REGEX Matching
         *****************************************************/
        match_val match_regex() {
            if (not check_string("r:")) {
                return {false, ""};
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

            auto sv = current_loc.sv.substr(0, count);
            fast_consume(count);

            return {true, std::move(sv)};
        }

        /******************************************************
         * STRING Matching
         *****************************************************/
        // does not consume
        bool check_string(std::string_view o) {
            return current_loc.sv.compare(0, o.size(), o) == 0;
        }

        /******************************************************
         * TYPE Match
         *****************************************************/
        match_val match_typestring() {
            if (not match_char('<')) {
                return {false, ""};
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

            auto sv = current_loc.sv.substr(0, count-1);
            // need to use consume since there may be embedded 
            // newlines.
            consume(count);

            return {true, std::move(sv)};

        }         

        /******************************************************
         * ACTION BLOCK Matching
         *****************************************************/
        match_val match_actionblock() {
            if (not check_string("<%{")) {
                return {false, ""};
            }
            fast_consume(3);
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
            auto sv = current_loc.sv.substr(0, count-3);
            // need to use consume since there may be embedded 
            // newlines.
            consume(count);

            return {true, std::move(sv)};

        }

        /******************************************************
         * SKIP Processing
         *****************************************************/
        bool skip() {

            // we'll be using this a lot.
            // So abbreviate it with a reference.
            auto &cl = current_loc;

            parse_loc comment_loc;

            bool skipping = true;
            int state = 0;
            while (skipping and not eoi()) {
                switch (state) {
                    case 0 : // Normal state
                        if (std::isspace(peek(0))) {
                            consume(1);
                        } else if (peek() == '/' and valid_pos(1)
                                and peek(1) == '/') {
                            comment_loc = cl;
                            fast_consume(2);
                            state = 1;
                        } else if (peek(0) == '/' and valid_pos(1)
                                and peek(1) == '*') {
                            comment_loc = cl;
                            fast_consume(2);
                            state = 2;
                        } else {
                            skipping = false;
                        }
                        break;
                    case 1: // Line comment
                        if (peek(0) == '\n') {
                            state = 0;
                        }
                        consume(1);
                        break;
                    case 2: // Block comment
                        if (peek(0) == '*' and valid_pos(1)
                                and peek(1) == '/') {
                            fast_consume(2);
                            state = 0;
                        } else {
                            consume(1);
                        }
                        break;
                }
            }

            if (state != 0) {
                record_error("Unterminated comment starting here", comment_loc);
                return false;
            }

            return true;
        }
    };

} // namespace yalr::parser

#endif
