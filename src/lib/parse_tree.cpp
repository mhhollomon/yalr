#include "parse_tree.hpp"

namespace yalr {

struct pp_visitor {
    std::ostream &strm;

    explicit pp_visitor(std::ostream &stream) : strm(stream) {}

    void operator()(terminal const &t) {
        strm << "TERM " << t.name
            << " " << (t.type_str? t.type_str->text : "" ) 
            << " " << t.pattern << " @assoc="
            << (t.associativity? t.associativity->text : "undef")
            << " @prec=" << (t.precedence ? t.precedence->text : "undef") << "\n";

    }
    void operator()(skip const &s) {
        strm << "SKIP " << s.name << " " << s.pattern << ";\n";
    }
    void operator()(rule const &r) {
    }
    void operator()(option const &o) {
    }
    void operator()(verbatim const &v) {
    }
    void operator()(precedence const &v) {
    }
    void operator()(associativity const &v) {
    }
    void operator()(termset const &v) {
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
