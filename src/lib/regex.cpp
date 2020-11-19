#include "utils.hpp"
#include <deque>
#include <list>
#include <iomanip>


std::string escape_char(char c) {
    return yalr::util::escape_char(c, false);
}

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

enum class operator_t {
    noop,       //        compile
    literal,    // parse, compile
    range,      // parse, compile
    close,      // parse
    nclose,     // parse
    plus,       // parse
    nplus,      // parse
    optn,       // parse
    noptn,      // parse
    concat,     // parse
    runion,     // parse
    split,      //        compile
    jump,       //        compile
    match,      //        compile
};

struct operation {
    operator_t oper;
    int op1;
    int op2;
};

struct regex_parser {

    std::string_view::iterator first_;
    std::string_view::const_iterator last_;
    bool success = true;

    // helps deal with mismatched closed parens
    int level_ = 0;

    std::list<operation> oper_list_;

    void add_oper(operator_t o, int p1 = 0, int p2 = 0) {
        oper_list_.emplace_back( operation{o, p1, p2} );
    }


    regex_parser(std::string_view::iterator f, 
            std::string_view::const_iterator l) : first_(f), last_(l) {}

    regex_parser(std::string_view s) : regex_parser(s.begin(), s.end()) {}

    //-----------------------------------------------------------------
    void dump_list(std::ostream &strm) {
        strm << "--------\n";
        for (auto const &oper : oper_list_) {
            switch (oper.oper) {
                case operator_t::literal :
                    strm << "litrl  '" << escape_char(static_cast<char>(oper.op1)) << "'\n";
                    break;
                case operator_t::range :
                    strm << "range  '" << escape_char(static_cast<char>(oper.op1)) << "', '";
                    strm << escape_char(static_cast<char>(oper.op2)) << "'\n";
                    break;
                case operator_t::close :
                    strm << "close\n";
                    break;
                case operator_t::nclose :
                    strm << "nclose\n";
                    break;
                case operator_t::plus :
                    strm << "plus\n";
                    break;
                case operator_t::nplus :
                    strm << "nplus\n";
                    break;
                case operator_t::optn :
                    strm << "optn\n";
                    break;
                case operator_t::noptn :
                    strm << "noptn\n";
                    break;
                case operator_t::concat :
                    strm << "concat\n";
                    break;
                case operator_t::runion :
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
            std::cerr << "Not at opening paren\n";
            success = false;
            return;
        }
        ++first_;
        parse_regex();
        c = *first_;
        if (c != ')') {
            std::cerr << "Not at closing paren\n";
            success = false;
            return;
        }
        ++first_;
    }

    //-----------------------------------------------------------------
    void add_ranges(std::set<char_range> const &ranges) {
        bool first = true;
        for (auto const &[l, h] : ranges) {
            add_oper(operator_t::range, l, h);
            if (first) {
                first = false;
            } else {
                add_oper(operator_t::runion);
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
                        std::cerr << "ERROR: character classs - invalid range - end must be larger than start\n";
                        return;
                    }
                    ranges.emplace(letter, end_letter);
                } else {
                    ranges.emplace(letter);
                }
            }
        }

        if (not close_seen) {
            std::cerr << "DIdn't see ']' closing the character class\n";
            return;
        }

        if (ranges.empty()) {
            std::cerr << "empty character class not supported\n";
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
                add_oper(operator_t::literal, c);
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
                add_oper(minimal ? operator_t::nclose : operator_t::close);
                break;
            case '?' :
                add_oper(minimal ? operator_t::noptn : operator_t::optn);
                break;
            case '+' :
                add_oper(minimal ? operator_t::nplus : operator_t::plus);
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
                            add_oper(operator_t::concat);
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
                    add_oper(operator_t::runion);
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
};


struct instruction {
    int label = 0;
    operator_t oper;
    int op1 = 0;
    int op2 = 0;

    instruction(const operation &o) :
        oper(o.oper), op1(o.op1), op2(o.op2) {}

    instruction(operator_t n, int one = 0, int two = 0) : oper(n), op1(one), op2(two) {};
    instruction() = default;
};

struct regex_compiler {

    std::list<instruction> instr_list;

    std::vector<instruction> program_;

    std::deque<std::list<instruction>> stack_;

    int last_label_ = 0;

    void dump_output(std::ostream &strm) {
        strm << "--------\n";
        for (auto const &oper : program_) {
            strm << std::setw(2) << oper.label << ": ";
            switch (oper.oper) {
                case operator_t::noop :
                    strm << "noop\n";
                    break;
                case operator_t::literal :
                    strm << "litrl  '" << escape_char(static_cast<char>(oper.op1)) << "'\n";
                    break;
                case operator_t::range :
                    strm << "range  '" << escape_char(static_cast<char>(oper.op1)) << "', '";
                    strm << escape_char(static_cast<char>(oper.op2)) << "'\n";
                    break;
                case operator_t::split :
                    strm << "split  " << oper.op1 << ", " << oper.op2 << "\n";
                    break;
                case operator_t::jump :
                    strm << "jump  " << oper.op1 << "\n";
                    break;
                case operator_t::match :
                    strm << "match\n";
                    break;
                default :
                    strm << "unkown " << oper.op1 << " " << oper.op2 << "\n";
            }
        }
        strm << "--------\n";
    }

    void compile_regex(const std::list<operation> &parse) {
        for (auto const &oper : parse) {
            switch(oper.oper) {
                case operator_t::literal : {
                    auto &iter = stack_.emplace_back();
                    iter.emplace_back(oper);
                    }
                    break;
                case operator_t::range : {
                    auto &iter = stack_.emplace_back();
                    iter.emplace_back(oper);
                    }
                    break;
                case operator_t::concat : {
                    // e1e2 :
                    //      e1
                    //      e2
                    //
                    auto second = stack_.back(); stack_.pop_back();
                    auto first  = stack_.back(); stack_.pop_back();
                    first.splice(first.end(), second);
                    stack_.emplace_back(std::move(first));
                    }
                    break;
                case operator_t::runion : {
                    // e1 | e2 :
                    //      split 1 , 2
                    //      1: e1
                    //      jmp 3
                    //      2 : e2
                    //      3 : noop
                    //
                    auto second = stack_.back(); stack_.pop_back();
                    auto first  = stack_.back(); stack_.pop_back();

                    first.begin()->label = ++last_label_;
                    auto & split = first.emplace_front(operator_t::split, 0, 0);
                    split.op1 = last_label_;

                    split.op2 = ++last_label_;
                    second.begin()->label = split.op2;

                    auto &noop = second.emplace_back(operator_t::noop);
                    noop.label = ++last_label_;
                    first.emplace_back(operator_t::jump, noop.label);
                    first.splice(first.end(), second);

                    stack_.emplace_back(std::move(first));
                    }
                    break;
                case operator_t::nclose :
                case operator_t::close : {
                    // e1* :
                    //      1 : split 2, 3
                    //      2 : e1
                    //          jump 1
                    //      3: noop
                    auto first  = stack_.back(); stack_.pop_back();
                    int split_label = ++last_label_;
                    int e1_label = ++last_label_;
                    int noop_label = ++last_label_;
                    first.begin()->label = e1_label;
                    if (oper.oper == operator_t::close) {
                        auto & split = first.emplace_front(operator_t::split, e1_label, noop_label);
                        split.label = split_label;
                    } else {
                        auto & split = first.emplace_front(operator_t::split, noop_label, e1_label);
                        split.label = split_label;
                    }
                    first.emplace_back(operator_t::jump, split_label);
                    auto & noop_instr = first.emplace_back(operator_t::noop);
                    noop_instr.label = noop_label;

                    stack_.emplace_back(std::move(first));
                    }

                    break;
                case operator_t::nplus :
                case operator_t::plus : {
                    // e1+ :
                    //      1: e1
                    //         split 1, 2
                    //      2: noop
                    auto first  = stack_.back(); stack_.pop_back();
                    int e1_label = ++last_label_;
                    int noop_label = ++last_label_;
                    first.begin()->label = e1_label;
                    if (oper.oper == operator_t::plus) {
                        first.emplace_back(operator_t::split, e1_label, noop_label);
                    } else {
                        first.emplace_back(operator_t::split, noop_label, e1_label);
                    }
                    first.emplace_back(operator_t::noop).label = noop_label;

                    stack_.emplace_back(std::move(first));
                    }
                    break;
                case operator_t::noptn :
                case operator_t::optn : {
                    // e1? :
                    //         split 1, 2
                    //      1: e1
                    //      2: noop
                    auto first  = stack_.back(); stack_.pop_back();
                    int e1_label = ++last_label_;
                    int noop_label = ++last_label_;

                    first.front().label = e1_label;
                    if (oper.oper == operator_t::optn) {
                        first.emplace_front(operator_t::split, e1_label, noop_label);
                    } else {
                        first.emplace_front(operator_t::split, noop_label, e1_label);
                    }
                    first.emplace_back(operator_t::noop).label = noop_label;

                    stack_.emplace_back(std::move(first));
                    }
                    break;
                default :
                    std::cerr << "Not implemented yet\n";
                    return;
            }
        }

        instr_list = stack_.back();
        instr_list.emplace_back(operator_t::match);

        relocate();

    }

    // turn the instuction list into a program by
    // turning labels into instruction offsets
    //
    void relocate() {
        program_.reserve(instr_list.size());

        std::map<int, int> label_to_pc;

        int pc = 0;
        for (auto const & x : instr_list) {
            if (x.label != 0) {
                label_to_pc.emplace(x.label, pc);
            }
            ++pc;
        }

        pc = -1;
        for (auto const & x : instr_list) {
            auto & y = program_.emplace_back(x);
            y.label = ++pc;
            switch (y.oper) {
                case operator_t::split:
                    y.op1 = label_to_pc.at(y.op1);
                    y.op2 = label_to_pc.at(y.op2);
                    break;
                case operator_t::jump :
                    y.op1 = label_to_pc.at(y.op1);
                    break;
                default :
                    // nothing to do
                    break;
            }
        }

    }
    struct thread_state {
        int pc;
        int length;

        thread_state(int p, int l) : pc(p), length(l) {}
        thread_state() = default;
    };

    struct thread_set {
        std::vector<thread_state> threads;

        bool empty() {
            return threads.empty();
        }

        void clear() {
            threads.clear();
        }

        void add_thread(int pc, int length) {
            for (auto const & x : threads) {
                if (pc == x.pc) {
                    return;
                }
            }
            threads.emplace_back(pc, length);
        }

        void print(std::ostream &strm) {
            strm << "thread set = ";
            if (empty()) {
                strm << "(empty)\n";
                return;
            }
            strm << "\n";
            for (auto const & x : threads) {
                strm << "      " << x.pc << " " << x.length << "\n";
            }
        }
    };

    void add_thread(int pc, int length, thread_set *threads) {
        auto const & current_instr = program_[pc];
        switch (current_instr.oper) {
            case operator_t::jump :
                add_thread(current_instr.op1, length, threads);
                break;
            case operator_t::noop :
                add_thread(pc+1, length, threads);
                break;
            case operator_t::split :
                add_thread(current_instr.op1, length, threads);
                add_thread(current_instr.op2, length, threads);
                break;
            default :
                threads->add_thread(pc, length);
                break;
        }
    }

    int match_length = -1;

    void check_for_match(thread_set *threads) {
        for (auto const &x : threads->threads) {
            if (program_.at(x.pc).oper == operator_t::match) {
                if (x.length > match_length) {
                    match_length = x.length;
                }
            }
        }
    }

    bool run(std::string_view input) {

        match_length = -1;
        thread_set *current_state = new thread_set();
        thread_set *next_state = new thread_set();
        thread_set *swap_temp;

        add_thread(0, 0, current_state);

        auto current_char_ptr = input.begin();

        /** Main loop
         *
         * Only advance the character pointer once per trip around
         */
        while(true) {
            next_state->clear();
            std::cout << "match_length = " << match_length << "\n";
            current_state->print(std::cout);
            std::cout << "next Char = " << *current_char_ptr << "\n";
            /** Thread loop
             * process one thread for the current character each iteration
             */
            for (auto const & state : current_state->threads) {
                bool stop_the_loop = false;

                int pc = state.pc;
                auto const & current_instr = program_[pc];

                switch(current_instr.oper) {
                    case operator_t::literal :
                        if (*current_char_ptr == static_cast<char>(current_instr.op1)) {
                            add_thread(pc+1, state.length+1, next_state);
                        }
                        break;
                    case operator_t::range :
                        if (*current_char_ptr >= static_cast<char>(current_instr.op1) and
                            *current_char_ptr <= static_cast<char>(current_instr.op2)) {
                            add_thread(pc+1, state.length+1, next_state);
                        }
                        break;
                    case operator_t::match :
                        if (state.length > match_length) {
                            match_length = state.length;
                        }
                        // lower priority threads get the shaft
                        stop_the_loop = true;
                        break;
                    case operator_t::jump :
                    case operator_t::split :
                    case operator_t::noop :
                        std::cerr << "saw an epsilon instruction - why didn't add_thread absorb it?\n";
                        return false;
                        break;
                    default :
                        std::cerr << "unkown instruction at pc = " << state.pc << "\n";
                        return false;
                }

                if (stop_the_loop) {
                    break;
                }

            } // threads

            if (next_state->empty()) {
                break;
            }

            ++current_char_ptr;
            if (current_char_ptr == input.end()) {
                break;
           }
            swap_temp = current_state;
            current_state= next_state;
            next_state = swap_temp;
        }


        next_state->print(std::cout);
        check_for_match(next_state);
        std::cout << "match_length = " << match_length << "\n";


        return match_length > -1;
    }


};

