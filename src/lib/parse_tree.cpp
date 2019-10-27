#include "parse_tree.hpp"

namespace yalr {

struct pp_visitor {
    std::ostream &strm;

    explicit pp_visitor(std::ostream &stream) : strm(stream) {}

    void operator()(terminal_stmt const &t) {
        strm << "TERM " << t.name
            << " " << (t.type_str? t.type_str->text : "" ) 
            << " " << t.pattern << " @assoc="
            << (t.associativity? t.associativity->text : "undef")
            << " @prec=" << (t.precedence ? t.precedence->text : "undef") << "\n";

    }
    void operator()(skip_stmt const &s) {
        strm << "SKIP " << s.name << " " << s.pattern << ";\n";
    }

    template<typename T>
    void operator()(const T & _) {
        /* don't care about terminals this time */
    }
};

std::ostream &parse_tree::pretty_print(std::ostream &strm) const {

    pp_visitor ppv{strm};

    for (auto const &s : statements) {
        std::visit(ppv, s);
    }

    return strm;
}

} // namespace
