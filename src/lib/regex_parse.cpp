#include "regex_parse.hpp"

#include "utils.hpp"
#include <deque>
#include <list>
#include <iomanip>
#include <map>
#include <set>


using yalr::util::escape_char;
/*
std::string escape_char(char c) {
    return yalr::util::escape_char(c, false);
}
*/

std::map<char, char> const escape_map = {
    { 'f', '\f' },
    { 'n', '\n' },
    { 'r', '\r' },
    { 't', '\t' },
    { 'v', '\v' },
    { '0', '\0' },
};

struct char_range {
    char low;
    char high;

    char_range(char c) : low(c), high(c) {};
    char_range() = default;
    char_range(char l, char h) : low(l), high(h) {};

    bool operator<(char_range const & o) const {
        return (low < o.low or (low == o.low and high < o.high));
    }
};

std::map<char, std::set<char_range>> const class_escape_map = {
    {'d', { {'0', '9'}, } },
    {'D', { {'\0', '0'-1, },
            {'9'+1, '\x7F'}, }, },

    {'s', { {'\t', '\r'},
            {' ',  ' ' }, }, },
    {'S', { {'\0', '\t'-1},
            {'\r'+1, ' '-1},
            {' '+1, '\x7F'}, }, },

    {'w', { {'0', '9'},
            {'A', 'Z' },
            {'a', 'z' },
            {'_', '_'}, }, },
    {'W', { {'\0', '0'-1, },
            {'9'+1, 'A'-1 },
            {'Z'+1, 'a'-1 },
            {'z'+1, '_'-1 },
            {'_'+1, '\x7f' }, }, },

};

std::set<char_range> const dot_ranges = {
    { { '\0', '\n'-1},
      { '\n'+1, '\x7F'}, }, 
};

/************************************************************************/
struct regex_parser {

    std::string_view::iterator first_;
    std::string_view::const_iterator last_;
    yalr::error_list &errors_;
    yalr::text_fragment const &regex_;
    bool success = true;

    // helps deal with mismatched closed parens
    int level_ = 0;

    rpn_ptr oper_list_;

    void add_oper(rpn_opcode_t o, int16_t p1 = 0, int16_t p2 = 0) {
        oper_list_->emplace_back( rpn_instruction{o, p1, p2} );
    }

    void add_error(std::string const &msg) {
        errors_.add(msg, regex_);
        success = false;
    }


    regex_parser(yalr::text_fragment const &r, yalr::error_list &e) : 
            first_(r.text.begin()), last_(r.text.end()), errors_(e),
            regex_(r) {
        oper_list_ = std::make_shared<rpn_list>();
    }

    //-----------------------------------------------------------------
    void dump_list(std::ostream &strm) {
        strm << "--------\n";
        for (auto const &oper : *oper_list_) {
            switch (oper.opcode) {
                case rpn_opcode_t::literal :
                    strm << "litrl  '" << escape_char(static_cast<char>(oper.op1)) << "'\n";
                    break;
                case rpn_opcode_t::range :
                    strm << "range  '" << escape_char(static_cast<char>(oper.op1)) << "', '";
                    strm << escape_char(static_cast<char>(oper.op2)) << "'\n";
                    break;
                case rpn_opcode_t::close :
                    strm << "close\n";
                    break;
                case rpn_opcode_t::nclose :
                    strm << "nclose\n";
                    break;
                case rpn_opcode_t::plus :
                    strm << "plus\n";
                    break;
                case rpn_opcode_t::nplus :
                    strm << "nplus\n";
                    break;
                case rpn_opcode_t::option :
                    strm << "options\n";
                    break;
                case rpn_opcode_t::noption :
                    strm << "noption\n";
                    break;
                case rpn_opcode_t::concat :
                    strm << "concat\n";
                    break;
                case rpn_opcode_t::join :
                    strm << "union\n";
                    break;
                default :
                    strm << "unkown " << oper.op1 << " " << oper.op2 << "\n";
            }
        }
        strm << "--------\n";
    }

    //-----------------------------------------------------------------
    void parse_parens() {
        char c = *first_;

        if (c != '(') {
            add_error("INTERNAL ERROR - Not at opening paren");
            return;
        }
        ++first_;
        parse_regex();
        c = *first_;
        if (c != ')') {
            add_error("Expecting closing parenthesis");
            return;
        }
        ++first_;
    }

    //-----------------------------------------------------------------
    void add_ranges(std::set<char_range> const &ranges) {
        bool first = true;
        for (auto const &[l, h] : ranges) {
            add_oper(rpn_opcode_t::range, l, h);
            if (first) {
                first = false;
            } else {
                add_oper(rpn_opcode_t::join);
            }
        }
    }
    //-----------------------------------------------------------------
    char parse_hex_esc() {
        char value = 0;

        bool value_computed = false;
        for (int i = 0 ; i < 2; ++ i) {
            value <<= 4;
            char c = *first_;
            if (c >= '0' and c <= '9') {
                value += c -'0';
            } else if ( c >= 'A' and c <= 'F') {
                value += c - 'A' + '\xA';
            } else if ( c >= 'a' and c <= 'f') {
                value += c - 'a' + '\xA';
            } else {
                // don't increment
                break;
            }
            ++first_;
            value_computed = true;
        }

        return (value_computed ? value : 'x');

    }
    //-----------------------------------------------------------------
    std::set<char_range> parse_esc_to_set() {
        ++first_;
        if (first_ == last_) {
            return { { '\\' }, };
        }

        auto siter = escape_map.find(*first_);
        if (siter != escape_map.end()) {
            ++first_;
            return { { siter->second }, };
        }

        auto miter = class_escape_map.find(*first_);
        if (miter != class_escape_map.end()) {
            ++first_;
            return miter->second;
        }

        if (*first_ == 'x') {
            ++first_;
            auto value = parse_hex_esc();
            return { { value, }, };
        };

        char c = *first_;
        ++first_;
        return { { c }, };
    }

    //-----------------------------------------------------------------
    bool parse_esc() {
        auto ranges = parse_esc_to_set();

        add_ranges(ranges);

        return true;
    }

    //-----------------------------------------------------------------
    void parse_dot() {
        ++first_;

        add_ranges(dot_ranges);
    }
    
    //-----------------------------------------------------------------
    void parse_char_class() {
        ++first_;

        bool negated_class = false;
        
        // check for it, but ignore for now
        if (*first_ == '^') {
            negated_class = true;
            ++first_;
        }

        std::set<char_range> ranges;

        bool done = false;
        bool close_seen = false;
        while (not done) {

            if (first_ == last_) {
                done = true;
                continue;
            }

            if (*first_ == ']') {
                done = true;
                close_seen = true;
                ++first_;
            } else if (*first_ == '\\') {
                auto new_ranges = parse_esc_to_set();
                ranges.insert(new_ranges.begin(), new_ranges.end());
            } else {
                auto letter = *first_;
                ++first_;
                if (*first_ == '-') {
                    ++first_;
                    auto end_letter = *first_;
                    ++first_;

                    if (end_letter <= letter) {
                        add_error("char class - invalid range - end must be larger than start");
                        return;
                    }
                    ranges.emplace(letter, end_letter);
                } else {
                    ranges.emplace(letter);
                }
            }
        }

        if (not close_seen) {
            add_error("missing ']' closing a character class");
            return;
        }

        if (ranges.empty()) {
            add_error("empty character class not supported");
            return;
        }

        add_ranges(ranges);

    }

    //-----------------------------------------------------------------
    bool parse_atom() {
        if (first_ == last_) { return false; }

        char c = *first_;

        switch(c) {
            case '(' :
                parse_parens();
                break;

            case '|' :
            case ')' :
                return false;
                break;

            case '\\' :
                parse_esc();
                break;

            case '.' :
                parse_dot();
                break;

            case '[' :
                parse_char_class();
                break;

            default :
                add_oper(rpn_opcode_t::literal, c);
                ++first_;
        }

        return true;
    }

    // --------------------------------------------------------
    bool parse_item() {

        static std::set<char> operators{ '*', '+', '?' };

        if (not parse_atom()) { return false; }

        if (first_ == last_) { return true; }

        char c = *first_;

        bool minimal = false;
        if (operators.count(c) > 0) {
            ++first_;
            if (*first_ == '?') {
                minimal = true;
                ++first_;
            }
        } else {
            return true;
        }

        switch(c) {
            case '*' :
                add_oper(minimal ? rpn_opcode_t::nclose : rpn_opcode_t::close);
                break;
            case '?' :
                add_oper(minimal ? rpn_opcode_t::noption : rpn_opcode_t::option);
                break;
            case '+' :
                add_oper(minimal ? rpn_opcode_t::nplus : rpn_opcode_t::plus);
                break;
        }

        return true;
    }

    // --------------------------------------------------------
    bool parse_alternate() {
        // alternate can be empty
        // e.g. 'a|' or '|a' is a weird but valid way
        // to say 'a?'
        //
        // loop this rather than recurse
        bool on_first = true;
        while (first_ != last_) {

            char c = *first_;

            switch(c) {
                case '|' :
                case ')' :
                    return true;
                default :
                    auto saw_item = parse_item();
                    if (saw_item) {
                        if (on_first) {
                            on_first = false;
                        } else {
                            add_oper(rpn_opcode_t::concat);
                        }
                    } else {
                        return true;
                    }
                    break;
            }
        }

        return true;
    }

    // --------------------------------------------------------
    // Top level parse routine
    //
    void parse_regex() {
        level_ += 1;

        parse_alternate();
        if (first_ != last_) {
            switch (*first_) {
                case '|' :
                    ++first_;
                    parse_regex();
                    add_oper(rpn_opcode_t::join);
                    break;
                case ')' :
                    return;
                default :
                    std::cerr << "seeing char = " << *first_ << " unexpected\n";
                    success = false;
                    return;
            }
        }

        level_ -= 1;
    }

    // ---------------------------------------------------
    // Parse a set string
    //
    void parse_string() {
        bool is_first = true;
        while (first_ != last_) {
            add_oper(rpn_opcode_t::literal, *first_);
            if (is_first) {
                is_first = false;
            } else {
                add_oper(rpn_opcode_t::concat);
            }
            ++first_;
        }
        success = true;
    }

};

/*******************************************************************************/
rpn_ptr regex2rpn(yalr::text_fragment const &regex, rpn_mode mode, yalr::error_list &errors) {
    auto parser = regex_parser(regex, errors);
    if (mode == rpn_mode::full_regex) {
        parser.parse_regex();
    } else {
        parser.parse_string();
    }
    if (parser.success) {
        return parser.oper_list_;
    } else {
        return nullptr;
    }
}
