#include "translate.hpp"
#include "options.hpp"

#include <fstream>
#include <ios>

namespace yalr::translate {

void output_prod(const yalr::production& p, std::ostream& strm) {
    strm << p.rule.name() << " ->";
    for (const auto& symb : p.items) {
        strm << " " << symb.sym.name();
    }
    strm << " .\n";

}

//NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void grammophone::output(const yalr::analyzer_tree& gr, CLIOptions &clopts) const {

    std::string outfilename;
    if (not clopts.output_file.empty()) {
        outfilename = clopts.output_file;
    } else {
        outfilename = std::string(gr.options.parser_class.get())
            + ".txt";
    }

    std::ofstream code_out(outfilename, std::ios_base::out);

    // grammophone uses the first production as the target
    // So, be sure to put our target out first.
    output_prod(gr.productions[int(gr.target_prod)], code_out);
    for (const auto& p : gr.productions) {
        if (p.prod_id != gr.target_prod) {
            output_prod(p, code_out);
        }
    }

}

} // namespace yalr::translate
