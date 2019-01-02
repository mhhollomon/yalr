#include "ast.hpp"


namespace yalr { namespace ast {

struct ast_print_visitor {
    std::ostream& strm;
    int indent = 0;

    ast_print_visitor(std::ostream& s = std::cout) : strm(s) {}

    void operator()(const grammar& g) {

        if (g.parser_class.has_value()) {
            strm << "parser class \"" << *g.parser_class << "\"\n";
        }

        for ( const auto& d : g.defs ) {
            std::visit(*this, d);
        }
    }

    void operator()(const rule_def& r) {
        strm << "rule " << r.name << " {\n";
        indent += 2;
        for (const alternative& a: r.alts) {
            (*this)(a);
        }
        indent -= 2;
        strm << "}\n";
    }

    void operator()(const alternative& a) {
        std::string pad1(indent, ' ');
        strm << pad1 << "=>";

        for (const std::string &s : a.pieces) {
            strm << ' ' << s;
        }

        strm << pad1 << ";\n";

    }

    void operator()(const terminal &t) {
        strm << "term " << t.name << ";\n";
    }

};

void pretty_print(grammar& g, std::ostream& strm) {

    ast_print_visitor vstr{strm};

    vstr(g);

}

}}
