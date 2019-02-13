#include "translate.hpp"

#include <fstream>
#include <ios>

namespace yalr::translate {

void output_prod(const analyzer::production& p, std::ostream& strm) {
    strm << p.rule.name() << " ->";
    for (const auto& symb : p.syms) {
        strm << " " << symb.name();
    }
    strm << " .\n";

}
void grammophone::output(const analyzer::grammar& gr, CLIOptions &clopts) const {

    std::string outfilename;
    if (not clopts.output_file.empty()) {
        outfilename = clopts.output_file;
    } else {
        outfilename = gr.parser_class + ".txt";
    }

    std::ofstream code_out(outfilename, std::ios_base::out);

    // grammophone uses the first production as the target
    // So, be sure to our target out first.
    output_prod(gr.productions[gr.target_prod], code_out);
    for (const auto& p : gr.productions) {
        if (p.prod_id != gr.target_prod) {
            output_prod(p, code_out);
        }
    }

}

} // namespace yalr::translate
