#include "codegen.hpp"
#include "template.hpp"
#include "analyzer.hpp"

#include <sstream>

namespace yalr::codegen {

using json = nlohmann::json;

using tablegen::action_type;

auto generate_state_data(const tablegen::lrstate& state,const tablegen::lrtable& lt) {
    auto sdata = json::object();
    sdata["id"] = state.id;

    auto actions_data = json::array();

    for (const auto& [sym, action] : state.actions) {
        auto adata = json::object();

        if (sym.name() == "$") {
            adata["token"] = "eoi" ;
        } else {
            adata["token"] = "TOK_" + sym.name();
        }

        switch (action.atype) {
            case action_type::shift :
                adata["type"] = "shift";
                adata["newstateid"] = action.new_state_id;
                break;
            case action_type::reduce : {
                    const auto& prod  = lt.productions[action.prod_id];
                    std::stringstream ss;
                    analyzer::pp printer{ss};
                    printer(prod);

                    adata["type"] = "reduce";
                    adata["production"] = ss.str();
                    adata["count"] = prod.syms.size();
                    if (prod.syms.empty()) {
                        adata["returnlevels"] = 0;
                    } else {
                        adata["returnlevels"] = prod.syms.size() - 1;
                    }

                    adata["symbol"] = "TOK_" + prod.rule.name() ;
                    adata["valuetype"] = prod.rule.getRuleInfo()->type_str;
                    // json .empty() checks for null (no value) rather than
                    // an empty string. This confuses clang-tidy.
                    //NOLINTNEXTLINE(readability-container-size-empty)
                    if (adata["valuetype"] != "") { 
                        adata["hasvaluetype"] = "Y";
                    } else {
                        adata["hasvaluetype"] = "N";
                    }
                }
                break;
            case action_type::accept :
                adata["type"] = "accept";
                break;
            default :
                //NOLINTNEXTLINE
                assert(false);
                break;
        }

        actions_data.push_back(adata);
    }

    sdata["actions"] = actions_data;

    auto gotos_data = json::array();
    for (const auto& g_iter : state.gotos) {
        auto gdata = json::object();
        gdata["symbol"] = "TOK_" + g_iter.first.name();
        gdata["stateid"] = g_iter.second;
        gotos_data.push_back(gdata);
    }
    sdata["gotos"] = gotos_data;

    return sdata;
}

void generate_code(const tablegen::lrtable& lt, std::ostream& outstrm) {

    inja::Environment env;
    env.set_expression("<%", "%>");

    json data;
    data["namespace"] = lt.parser_class;


    // dump the tokens
    // We need two separate lists of tokens.
    // The first is the tokens to are returned by the
    // lexer or rule names used by the Parser.
    // The second list is the terms and skips that the
    // Lexer needs to generate the matching rules.
    // While we're at it, gather the name for types we'll need
    // for the semantic value variant.

    // List of terms/skips that need to matched against.
    std::vector<SymbolTable::symbol>terms;

    // names and values for the token enum.
    auto enum_entries = json::array();
    auto semantic_actions = json::array();

    // set of type names
    std::set<std::string>type_names;

    for (const auto &[sname, sym] : lt.syms) {
        std::string tok_name = "TOK_" + sname;
        if (sym.isterm()) {
            // terms go in the enum and the term list
            // type names go in the set
            if (sname == "$") {
                enum_entries.push_back(json::object({ 
                        { "name" , "eoi"}, {"value", sym.id() } }));
            } else {
                enum_entries.push_back(json::object({ 
                        { "name" , tok_name }, {"value", sym.id() } }));
                terms.push_back(sym);
                const ast::terminal* info_ptr = sym.getTerminalInfo();
                // NOLINTNEXTLINE
                assert(info_ptr != nullptr);// should never happen
                if (info_ptr->type_str != "void") {
                    type_names.insert(info_ptr->type_str);
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
                    { "name" , tok_name }, {"value", sym.id() } }));
        } else if (sym.isskip()) {
            // Skips only go in the term list
            terms.push_back(sym);
        } else {
            //NOLINTNEXTLINE
            assert(false);
        }
    }

    data["semantic_actions"] = semantic_actions;

    enum_entries.push_back(json::object({ 
            { "name" , "undef"}, {"value", -1 } }));
    enum_entries.push_back(json::object({ 
            { "name" , "skip"}, {"value", -10 } }));

    data["enums"] = enum_entries;

    auto type_arr = json::array();
    for (auto const& t : type_names ) {
        type_arr.push_back(t);
    }

    data["types"] = type_arr;



    // Sort by id - which should be the same as 
    // the order they were defined in the grammar
    // spec.
    std::sort(terms.begin(), terms.end());

    auto patterns = json::array();

    for (const auto& sym : terms) {
        auto tdata = json::object();

        const ast::terminal* info_ptr = sym.getTerminalInfo();

        if (info_ptr == nullptr) {
            info_ptr = sym.getSkipInfo();
        }

        std::string pattern = info_ptr->pattern.empty() ?
            "' '" : info_ptr->pattern;

        if (pattern[0] == '\'') {
            tdata["matcher"] = "string_matcher";
            tdata["pattern"] = 
                "R\"%_^xx(" + pattern.substr(1, pattern.size()-2) + ")%_^xx\"" ;
        } else {
            tdata["matcher"] = "regex_matcher";
            tdata["pattern"] =
                "R\"%_^xx(" + pattern.substr(2)  + ")%_^xx\"" ;
        }

        if (sym.isterm()) {
            tdata["token"] = "TOK_" + info_ptr->name;
        } else {
            tdata["token"] = "skip" ;
        }

        patterns.push_back(tdata);

    }

    data["patterns"] = patterns;


    // define the Parser class

    auto states_array = json::array();
    for (const auto& state : lt.states) {
        states_array.push_back(generate_state_data(state, lt));
    }
    data["states"] = states_array;

    /*
    std::cout << "----------------------------\n";
    std::cout << data.dump(4) << std::endl;
    std::cout << "----------------------------\n";
    */

    // write the template
    auto main_templ = env.parse(main_template);
    env.render_to(outstrm, main_templ, data);

}

} //namespace yalr::codegen
