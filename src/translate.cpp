#include "translate.hpp"

namespace yalr::translate {

void output_prod(const analyzer::production& p, std::ostream& strm) {
    strm << p.rule.name() << " ->";
    for (const auto& symb : p.syms) {
        strm << " " << symb.name();
    }
    strm << " .\n";

}
void grammophone::output(std::ostream& strm) const {

    output_prod(g.productions[g.target_prod], strm);
    for (const auto& p : g.productions) {
        if (p.prod_id != g.target_prod) {
            output_prod(p, strm);
        }
    }

}

} // namespace yalr::translate
