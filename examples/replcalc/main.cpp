#include "parser.hpp"
#include "cxxopts.hpp"
#include <string>
#include <fstream>
#include <deque>

struct cli_options {
    std::string file;
    bool parser_debug;
    bool lexer_debug;
};

cli_options parse_commandline(int argc, char**argv) {
    cli_options clopts;
    bool do_help;
    bool both_debug;

    auto options = cxxopts::Options{"replcalc", "calclator with a REPL loop"};

    options.add_options()
            ("h,help", "Get help message", cxxopts::value(do_help))
            ("f,file", "input from file", cxxopts::value(clopts.file))
            ("l,lexer-debug", "", cxxopts::value(clopts.lexer_debug))
            ("p,parser-debug", "", cxxopts::value(clopts.parser_debug))
            ("b,both-debug", "debug both parser and lexer", cxxopts::value(both_debug))
            ;

    try {
        auto results = options.parse(argc, argv);

        if (do_help) {
            std::cout << options.help() << "\n";
            exit (0);
        }

        if (both_debug) {
            clopts.lexer_debug = true;
            clopts.parser_debug = true;
        }

    } catch(const cxxopts::OptionException& e) {
        std::cerr << "Command line error: " << e.what() << "\n\n";
        std::cerr << options.help() << "\n";
        exit(1);
    }

    return clopts;

}

struct interactive_source {

    struct node {
        node *next_ = nullptr;
        node *prev_ = nullptr;
        std::string data_;

        node(std::string data) : data_(data) {}
    };

    node *head_ = nullptr;
    node *tail_ = nullptr;
    bool saw_eof_ = false;

    ~interactive_source() {
        node *current = tail_;

        while (current != nullptr) {
            auto upone = current->prev_;
            delete current;
            current = upone;
        }
    }

    interactive_source() = default;

    struct iter {
        interactive_source *parent_ = nullptr;
        node *current_node_ = nullptr;
        std::string::const_iterator string_iter_;
        bool is_end = false;

        using difference_type = std::size_t;
        using value_type = char;
        using pointer = const char*;
        using reference = const char&;
        using iterator_category = std::forward_iterator_tag;

        iter(interactive_source *parent) : parent_(parent), current_node_(parent->head_) {
            string_iter_ = current_node_->data_.begin();
            //std::cerr << this << ": New iterator to " << parent << "\n";
        }

        iter() {
            //std::cerr << this << ": New default iterator\n";
        }

        iter(const iter &o) : parent_(o.parent_), current_node_(o.current_node_), string_iter_(o.string_iter_),
            is_end(o.is_end) {
                
            //std::cerr << this << ": Copy Contructor from " << &o << "\n";
        }

        iter &operator=(const iter&o) {

            parent_ = o.parent_;
            current_node_ = o.current_node_;
            string_iter_ = o.string_iter_;
            is_end = o.is_end;

            //std::cerr << this << ": Copy Assignment from " << &o << "\n";

            return *this;
        }


        const char operator*() const {
            return *string_iter_;
        }

        iter &operator++() {
            //std::cerr << "++ " << this << ": start = '" << *string_iter_ << "'\n";
            ++string_iter_;
            if (string_iter_ == current_node_->data_.end()) {
                //std::cerr << "++ " << this << ": At the end of this buffer\n";
                if (current_node_ == parent_->tail_) {
                    //std::cerr<< "++ " << this << ": trying to get a new line of text\n";
                    get_next_line(*parent_);
                } else {
                    //std::cerr << "More buffers to try\n";
                }
                current_node_ = current_node_->next_;
                if (current_node_ == nullptr) {
                    is_end = true;
                    //std::cerr << "At the end of input\n";
                    string_iter_ = std::string::const_iterator();
                } else {
                    string_iter_ = current_node_->data_.begin();
                }
            } else {
                //std::cerr << "Not at the end of this buffer\n";
            }

            /*
            if (is_end) {
                std::cerr << "At the end of input\n";
            } else {
                std::cerr << "++ end  = '" << *string_iter_ << "'\n";
            }
            */

            return *this;
        }

        iter &operator--() {
            if (is_end) {
                current_node_ = parent_->tail_;
                string_iter_ = current_node_->data_.begin() + current_node_->data_.length()-1;

            } else if  (string_iter_ == current_node_->data_.begin()) {
                current_node_ = current_node_->prev_;
                // should atch the nullptr case, but just let segv
                string_iter_ = current_node_->data_.begin() + current_node_->data_.length() - 1;
            } else {
                --string_iter_;
            }
            return *this;
        }

        bool operator ==(const iter &o) const {
            if (is_end and o.is_end) return true;
            if (is_end != o.is_end) return false;

            return (current_node_ == o.current_node_ && string_iter_ == o.string_iter_);
        }

        bool operator !=(const iter&o) const {
            return (not(*this == o));
        }

        iter& operator +=(int l) {
            while (l > 0) {
                --l;
                ++(*this);
            }
            return *this;
        }

        iter operator+(long l) const {
            auto retval = *this;
            retval += l;
            return retval;
        }

        difference_type operator-(const iter&o) const {
            // Currently assumes that we are the later iterator
            // Assumes o is not end()
            // Assumes that both point into the same parent
            if (*this == o) return 0;

            if (is_end and o.is_end) return 0;

            node *guard;
            if (is_end) {
                guard = nullptr;
            } else {
                guard = current_node_;
            }

            difference_type retval = 0;
            for (auto c = o.current_node_; c != guard; c = c->next_) {
                retval += c->data_.length();
            }

            retval -= o.string_iter_ - o.current_node_->data_.begin();

            if (not is_end) {
                retval += string_iter_ - current_node_->data_.begin();
            }


            return retval;
        }

    };

    iter begin() {
        if (tail_ == nullptr) {
            get_next_line(*this);
        }

        return iter{this};
    }

    iter end() {
        iter retval{this};
        retval.is_end = true;
        return retval;
    }

    void add_node(std::string s) {
        node * n = new node(s);

        if (tail_) {
            tail_->next_ = n;
            n->prev_ = tail_;
            tail_ = n;
        } else {
            head_ = tail_ = n;
        }
    }
    void static get_next_line( interactive_source &is) {
        std::string data;

        if (is.saw_eof_) return;

        std::cout << "calc: ";
        std::flush(std::cout);
        std::getline(std::cin, data);
        if (std::cin.eof()) {
            is.saw_eof_ = true;
        }

        if (not std::cin.eof() ) {
            //std::cerr << "NO EOF\n";
            data.push_back('\n');
            //std::cerr << "adding buffer : " << data << "length = " << data.length() <<"\n";
            is.add_node(data);
        } else if (not data.empty()) {
            //std::cerr << "SAW EOF but data\n";
            //std::cerr << "adding buffer : " << data << "length = " << data.length() <<"\n";
            is.add_node(data);
        } else {
            //std::cerr << "SAW EOF NO data\n";
        }
    }



};


int main(int argc, char*argv[]) {

    std::string input;
    auto clopts = parse_commandline(argc, argv);

    if (not clopts.file.empty()) {
        // slurp the entire file contents and parse as a "batch".
        //
        std::ifstream fstrm{clopts.file};
        if (fstrm) {
            std::ostringstream os;
            os << fstrm.rdbuf();
            input = os.str();
        } else {
            std::cerr << "Failed to open file '" << input << "'\n";
            exit(1);
        }

        YalrParser::Lexer lexer(input.cbegin(), input.cend());
        lexer.debug = clopts.lexer_debug;

        auto parser = YalrParser::Parser(lexer);
        parser.debug = clopts.parser_debug;

        if (parser.doparse()) {
            //std::cout << "Input matches grammar!\n";
            return 0;
        } else {
            std::cout << "Input does NOT match grammar\n";
            return 1;
        }
    } else {
        interactive_source is;

        YalrParser::Lexer lexer(is.begin(), is.end());
        lexer.debug = clopts.lexer_debug;

        auto parser = YalrParser::Parser(lexer);
        parser.debug = clopts.parser_debug;

        if (parser.doparse()) {
            //std::cout << "Input matches grammar!\n";
            return 0;
        } else {
            std::cout << "Input does NOT match grammar\n";
            return 1;
        }
    }
}
