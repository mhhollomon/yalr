#include "translate.hpp"
#include "options.hpp"
#include "codegen/fsm_builders.hpp"

#include <fstream>
#include <ios>
#include "utils.hpp"

namespace yalr::translate {

void output_prod(const yalr::production& p, std::ostream& strm) {
    strm << p.rule.name() << " ->";
    for (const auto& symb : p.items) {
        strm << " " << symb.sym.name();
    }
    strm << " .\n";

}

//NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void grammophone::output(const yalr::analyzer_tree& gr, cli_options &clopts) const {

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
//
//NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void lexer_graph::output(const yalr::analyzer_tree& gr, cli_options &clopts) const {

    std::string outfilename_base;
    if (not clopts.output_file.empty()) {
        outfilename_base = clopts.output_file;
    } else {
        outfilename_base = std::string(gr.options.lexer_class.get());
    }

    auto nfa = codegen::build_nfa(gr.symbols);

    {
        std::ofstream code_out(outfilename_base + ".nfa.gv", std::ios_base::out);


        code_out << "digraph lexer {\n";

        for(auto const &[id, state] : nfa->states_) {
            if (state.accepting_) {
                auto s = gr.symbols.find(state.accepted_symbol_);
                code_out << "S" << id << 
                    "[shape=doublecircle, label=\"TOK_" << s->token_name() << "\"];\n";
            }
            for (auto const &[alpha, new_state_id] : state.transitions_) {
                std::string edge_label;
                if (alpha.index() == 0) {
                    edge_label = "&epsilon;"sv;
                } else {
                    auto esc_seq = util::escape_char(std::get<1>(alpha));
                    if (esc_seq[0] == '\\') {
                        esc_seq = std::string("\\\\") + esc_seq;
                    }
                    edge_label = esc_seq;
                }

                code_out << "S" << id << " -> S"  << new_state_id << "[label=\"" << edge_label << "\"];\n";
            }
        }
        

        code_out << "}";
    }

    auto dfa = codegen::build_dfa(*nfa);

    {
        std::ofstream code_out(outfilename_base + ".dfa.gv", std::ios_base::out);


        code_out << "digraph lexer {\n";

        for(auto const &[id, state] : dfa->states_) {
            if (state.accepting_) {
                //auto s = gr.symbols.find(state.accepted_symbol);

                //code_out << "S" << id << "[shape=doublecircle, label=\"TOK_" << s->token_name() << "\"];\n";
                code_out << "S" << id << "[shape=doublecircle];\n";
            }
            for (auto const &[alpha, new_state_id] : state.transitions_) {
                    std::string edge_label;
                    auto esc_seq = util::escape_char(alpha);
                    if (esc_seq[0] == '\\') {
                        esc_seq = std::string("\\\\") + esc_seq;
                    }
                    edge_label = esc_seq;
                code_out << "S" << id << " -> S"  << new_state_id << "[label=\"" << 
                    esc_seq << "\"];\n";
            }
        }
        

        code_out << "}";
    }
}

} // namespace yalr::translate
