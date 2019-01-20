#include "codegen.hpp"
#include "template.hpp"

#include "analyzer.hpp"

namespace yalr::codegen {

using json = nlohmann::json;

using tablegen::action_type;

void generate_state_function( const tablegen::lrstate& state, 
        const tablegen::lrtable& lt, std::ostream& outstrm) {

    analyzer::pp printer{outstrm};
    std::string pfx = "    ";

    outstrm << pfx << "rettype state" << state.id << "() {\n";
    outstrm << pfx << pfx << "std::cerr << \"entering state " << state.id <<
        "\\n\";\n";

    outstrm << R"T(
        rettype retval;

        switch (la.toktype) {
)T";

    for (const auto& [sym, action] : state.actions) {

        outstrm << pfx << pfx << pfx << "case ";
        if (sym.name() == "$") {
            outstrm << "eoi" ;
        } else {
            outstrm << "TOK_" << sym.name();
        }
        outstrm << " :\n";
        switch (action.atype) {
            case action_type::shift :
                outstrm << pfx << pfx << pfx << pfx << 
                    "shift(); retval = state" <<
                    action.new_state_id << "(); break;\n";
                break;
            case action_type::reduce : {
                    const auto& prod  = lt.productions[action.prod_id];
                    outstrm << pfx << pfx << pfx << pfx <<
                        "std::cerr << \"Reducing by : ";
                    printer(prod);

                    outstrm << "\\n\";\n";
                    outstrm << pfx << pfx << pfx << pfx << "reduce(" <<
                            prod.syms.size() << ", TOK_" << prod.rule.name() << ");\n";
                    if (not prod.syms.empty()) {
                        outstrm << pfx << pfx << pfx << pfx <<
                            "return { state_action::reduce, " <<
                            (prod.syms.size()-1) << ", " << 
                            prod.rule.id() << " };\n";
                    } else {
                        outstrm << pfx << pfx << pfx << pfx <<
                            "retval = { state_action::reduce, 0, " <<
                            prod.rule.id() << " };\n";
                    }
                    outstrm << pfx << pfx << pfx << pfx << "break;\n";
                }
                break;

            case action_type::accept :
                outstrm << pfx << pfx << pfx << pfx << 
                    "std::cerr << \"Accepting\\n\";\n";
                outstrm << pfx << pfx << pfx << pfx << 
                    "return { state_action::accept, 0 };\n";
                break;

            default :
                //NOLINTNEXTLINE
                assert(false);
                break;
        }
    }

    outstrm << pfx << pfx << pfx << "default: return { state_action::error };\n";
    outstrm << pfx << pfx << "};\n";

    outstrm << R"T(
        while (retval.action == state_action::reduce) {
            if (retval.depth > 0) {
                retval.depth -= 1;
)T";
    outstrm << pfx << pfx << pfx << pfx << "std::cerr << \"Leaving state " << 
        state.id << " in middle of reduce\\n\";";

    outstrm << R"T(
                return retval;
            }
)T";

    if (not state.gotos.empty()) {
        outstrm << pfx << pfx << pfx << "switch(retval.symbol) {\n";
        for (const auto& g_iter : state.gotos) {
            outstrm << pfx << pfx << pfx << pfx <<
                "case " << g_iter.first.id() << 
                " : retval = state" << g_iter.second <<
                "(); break;\n";
        }
        outstrm <<  pfx << pfx << pfx << "}\n";
    }

    outstrm << R"T(
        }
        return retval;
    }
)T";
}

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
                    adata["type"] = "reduce";
                    adata["production"] = "Blah";
                    adata["count"] = prod.syms.size();
                    adata["returnlevels"] = prod.syms.size() - 1;

                    adata["symbol"] = "TOK_" + prod.rule.name() ;
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

    json data;
    data["namespace"] = lt.parser_class;


    // dump the tokens
    // We need two separate lists of tokens.
    // The first is the tokens to are returned by the
    // lexer or rule names used by the Parser.
    // The second list is the terms and skips that the
    // Lexer needs to generate the mathing rules.

    // List of terms/skips that need to matched against.
    std::vector<SymbolTable::symbol>terms;

    // names and values for the token enum.
    auto enum_entries = json::array();

    for (const auto &[sname, sym] : lt.syms) {
        if (sym.isterm()) {
            // terms go in the enum and the term list
            if (sname == "$") {
                enum_entries.push_back(json::object({ 
                        { "name" , "eoi"}, {"value", sym.id() } }));
            } else {
                enum_entries.push_back(json::object({ 
                        { "name" , "TOK_" + sname }, {"value", sym.id() } }));
                terms.push_back(sym);
            }
        } else if (sym.isrule()) {
            // rules only go in the enum
            enum_entries.push_back(json::object({ 
                    { "name" , "TOK_" + sname }, {"value", sym.id() } }));
        } else if (sym.isskip()) {
            // Skips only go in the term list
            terms.push_back(sym);
        } else {
            //NOLINTNEXTLINE
            assert(false);
        }
    }

    enum_entries.push_back(json::object({ 
            { "name" , "undef"}, {"value", -1 } }));
    enum_entries.push_back(json::object({ 
            { "name" , "skip"}, {"value", -10 } }));

    data["enums"] = enum_entries;



    // Sort by id - which should be the same as 
    // the order they were defined in the grammar
    // spec.
    std::sort(terms.begin(), terms.end());

    auto patterns = json::array();

    for (const auto& sym : terms) {
        const ast::terminal* info_ptr = sym.getTerminalInfo();

        if (info_ptr == nullptr) {
            info_ptr = sym.getSkipInfo();
        }

        std::string pattern = info_ptr->pattern.empty() ?
            "\"\"" : info_ptr->pattern;

        // tactical fix to deal with the single quoted patterns until I can
        // write the updated lexer code.
        if (pattern[0] == '\'') {
            pattern = "R\"%_^xx(" + pattern.substr(1, pattern.size()-2) + ")%_^xx\"" ;
        } else {
            pattern = "R\"%_^xx(" + pattern.substr(2)  + ")%_^xx\"" ;
        }

        std::string token_string;
        if (sym.isterm()) {
            token_string = "TOK_" + info_ptr->name;
        } else {
            token_string = "skip" ;
        }

        patterns.push_back(json::object({ 
                { "pattern" , pattern}, {"token", token_string } }));

    }

    data["patterns"] = patterns;


    // define the Parser class

    auto states_array = json::array();
    for (const auto& state : lt.states) {
        states_array.push_back(generate_state_data(state, lt));
    }
    data["states"] = states_array;

    // write the template
    auto main_templ = env.parse(main_template);
    env.render_to(outstrm, main_templ, data);

}

} //namespace yalr::codegen
