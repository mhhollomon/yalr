#include "codegen.hpp"

#include "code_renderer.hpp"
#include "templates/main_func.hpp"
#include "templates/preamble.hpp"
#include "templates/lexer.hpp"
#include "templates/postlude.hpp"
#include "codegen/fsm_builders.hpp"
#include "utils.hpp"

#include <iostream>
#include <iomanip>
#include <ctime>
#include <sstream>

namespace yalr {

namespace codegen {

    using json = nlohmann::json;

    constexpr int SKIP_TOKEN_VALUE = -10;

    json generate_preamble_data( const analyzer_tree &tree) {

		json retval = json::object();

        // ------- Verbatim map ------------------
        json verbatims = json::object();

		for (auto v : verbatim_locations) {
			auto pieces = json::array();
			auto range =  tree.verbatim_map.equal_range(std::string(v));

			for (auto t = range.first; t != range.second; ++t) {
				pieces.push_back(std::string(t->second));
			}
			auto loc = std::string(v);
			loc.replace(loc.find('.'), 1, "_");
			verbatims[loc] = pieces;
		}
        retval["verbatim"] = verbatims;

        retval["namespace"] = std::string(tree.options.code_namespace.get());
        retval["parserclass"] = std::string(tree.options.parser_class.get());
        retval["lexerclass"] = std::string(tree.options.lexer_class.get());
        //
        // ---------- "header" ---------------------------
        {

            auto t = std::time(nullptr);
            auto tm = *std::localtime(&t);

            std::ostringstream oss;
            oss << tree.version_string << " at " << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
            retval["header"] = oss.str();
        }

        return retval;


    }

    /*--------------------------------------------------------------------*/

    void generate_lexer_data(const analyzer_tree &tree, json &data) {
        // dump the tokens
        // We need two separate lists of tokens.
        // The first is the tokens that are returned by the
        // lexer or rule names used by the Parser.
        // The second list is the terms and skips that the
        // Lexer needs to generate the matching rules.
        // While we're at it, gather the name for types we'll need
        // for the semantic value variant.

        // List of terms/skips that need to matched against.
        std::vector<symbol>terms;

        // names and values for the token enum.
        auto enum_entries = json::array();
        auto semantic_actions = json::array();

        // set of type names
        std::set<std::string>type_names;

        int enum_value = -1;

        for (const auto &[_, sym] : tree.symbols) {
            std::string tok_name = "TOK_" + std::string(sym.token_name());
            if (sym.isterm()) {
                // terms go in the enum and the term list
                // type names go in the set
                if (sym.name() == "$") {
                    enum_entries.push_back(json::object({ 
                            { "name" , "eoi"}, {"value", ++enum_value }, {"debugname", "eoi"} }));
                } else {
                    enum_entries.push_back(json::object({ 
                            { "name" , tok_name }, {"value", ++enum_value }, {"debugname", sym.name() } }));
                    terms.push_back(sym);
                    const auto* info_ptr = sym.get_data<symbol_type::terminal>();
                    yassert(info_ptr, "could not get data pointer for terminal");
                    if (info_ptr->type_str != "void") {
                        type_names.insert(std::string(info_ptr->type_str));
                    }
                    if (not info_ptr->action.empty()) {
                        semantic_actions.push_back(json::object({
                                    { "token", tok_name }, 
                                    { "block" , info_ptr->action },
                                    { "type"  , info_ptr->type_str }
                                    }));
                    }

                }
            } else if (sym.isrule()) {
                // rules only go in the enum
                enum_entries.push_back(json::object({ 
                        { "name" , tok_name }, {"value", ++enum_value }, {"debugname", sym.name() } }));
            } else if (sym.isskip()) {
                // Skips only go in the term list
                terms.push_back(sym);
                // put a dummy entry in so names align
                enum_entries.push_back(json::object({
                            { "name" , tok_name }, {"value", ++enum_value }, {"debugname", sym.name() } }));

            } else {
                yfail("Unknown symobl type");
            }
        }

        data["semantic_actions"] = semantic_actions;

        enum_entries.push_back(json::object({ 
                { "name" , "undef"}, {"value", -1 } }));
        enum_entries.push_back(json::object({ 
                { "name" , "skip"}, {"value", SKIP_TOKEN_VALUE } }));

        data["enums"] = enum_entries;

        auto type_arr = json::array();
        for (auto const& t : type_names ) {
            type_arr.push_back(t);
        }

        data["types"] = type_arr;



        // Sort by id - which should be the same as 
        // the order they were defined in the grammar
        // spec.
        //std::sort(terms.begin(), terms.end());


        auto patterns = json::array();

        for (const auto& sym : terms) {
            auto tdata = json::object();

            std::string pattern;
            case_type ct;
            pattern_type pt;
            bool glbl;
            const auto* info_ptr = sym.get_data<symbol_type::terminal>();

            if (info_ptr == nullptr) {
                const auto *skip_ptr = sym.get_data<symbol_type::skip>();
                pattern = std::string(skip_ptr->pattern);
                pt = skip_ptr->pat_type;
                ct = skip_ptr->case_match;
                // skips are always global
                glbl = true;
            } else {
                pattern =  std::string(info_ptr->pattern);
                pt = info_ptr->pat_type;
                ct = info_ptr->case_match;
                glbl = info_ptr->is_global;
            }

            tdata["flags"] = " ";
            // with dfa - ignore string matches since they are in 
            // the dfa
            if (pt == pattern_type::string) {
                continue;
            } else {
                tdata["matcher"] = "regex_matcher";
                tdata["pattern"] = "R\"%_^xx(" + pattern  + ")%_^xx\"" ;
                if (ct == case_type::fold) {
                    tdata["flags"] = ", std::regex::icase";
                }
            }

            tdata["rank"] = int(sym.id());
            if (sym.isterm()) {
                tdata["token"] = "TOK_" + std::string(info_ptr->token_name);
            } else {
                tdata["token"] = "skip" ;
            }

            tdata["is_global"] = glbl;

            patterns.push_back(tdata);

        }

        data["patterns"] = patterns;
        data["pattern_count"] = patterns.size();

    }
    /*--------------------------------------------------------------------*/

    void generate_dfa_data(dfa_machine const &dfa, symbol_table const &symtab, json &data) {
        // ordered_map (the default) returns things in key order - which we want

        int state_count = 0;
        auto state_info = json::array();
        int trans_count = 0;
        auto trans_info = json::array();

        for (auto const &[id, state] :dfa.states_) {
            if (state.accepting_) {
                for (auto token_id : state.accepted_symbol_) {
                    auto astate = json::object();
                    astate["id"] = int(id);
                    auto token_name = std::string{symtab.find(token_id)->token_name()};
                    astate["accepted"] = "TOK_" + token_name;
                    astate["rank"] = int(token_id);
                    ++state_count;
                    state_info.push_back(astate);
                }
            } else {
                auto astate = json::object();
                astate["id"] = int(id);
                astate["accepted"] = "token_type::undef";
                astate["rank"] = -1;
                ++state_count;
                state_info.push_back(astate);
            }
            // look at this states transitions

            for (auto const [input, state_id] : state.transitions_) {
                auto atrans = json::object();
                atrans["id"] = int(id);

                std::string char_input = "'" + util::escape_char(input) + "'";
                atrans["input"] = char_input;
                atrans["next_state"] = int(state_id);
                ++trans_count;
                trans_info.push_back(atrans);

            }

        }

        data["dfa"] = json::object();
        data["dfa"]["state"] = state_info;
        data["dfa"]["state_count"] = state_count;
        data["dfa"]["transitions"] = trans_info;
        data["dfa"]["trans_count"] = trans_count;
        data["dfa"]["start_state"] = int(dfa.start_state_);

    }

} // namespace codegen

std::unique_ptr<algo::gen_results> 
generate_code(
    std::shared_ptr<yalr::algo::parser_generator> gen, const analyzer_tree &tree, std::ostream &strm) {

    auto data = codegen::generate_preamble_data(tree);

    auto lexer_nfa = codegen::build_nfa(tree.symbols);

    auto lexer_dfa = codegen::build_dfa(*lexer_nfa);

    codegen::generate_lexer_data(tree, data);

    codegen::generate_dfa_data(*lexer_dfa, tree.symbols, data);

    auto cr = code_renderer(strm);

    cr.render(yalr::codegen::codegen_preamble, data);

    cr.render(yalr::codegen::lexer_template, data);

    auto retval = gen->generate_code(cr);

    cr.render(yalr::codegen::postlude_template, data);
    
    if (tree.options.code_main.get()) {
        cr.stream() << yalr::codegen::gen_main_code;
    }

    return retval;
}

} // namespace yalr
