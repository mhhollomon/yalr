#include "codegen.hpp"
#include "template.hpp"
#include "utils.hpp"
#include "template_genmain.hpp"
#include "analyzer_tree.hpp"

#include "yassert.hpp"

#include <iostream>
#include <sstream>

namespace yalr {

using json = nlohmann::json;

std::ostream& production_printer(std::ostream& strm, const production& p) {
    strm << "[" << p.prod_id << "] " << 
        p.rule.name() << "(" << p.rule.id() << ") =>";
    for (const auto& i : p.items) {
        strm << " " << i.sym.name() << "(" << i.sym.id() << ")";
    }

    return strm;
}

auto generate_state_data(const lrstate& state,const lrtable& lt) {
    auto sdata = json::object();
    sdata["id"] = int(state.id);

    auto actions_data = json::array();

    for (const auto& [sym, action] : state.actions) {
        auto adata = json::object();

        if (sym.name() == "$") {
            adata["token"] = "eoi" ;
        } else {
            adata["token"] = "TOK_" + std::string(sym.token_name());
        }

        switch (action.type) {
            case action_type::shift :
                adata["type"] = "shift";
                adata["newstateid"] = int(action.new_state_id);
                break;
            case action_type::accept :
                adata["type"] = "accept";
                break;
            case action_type::reduce : {
                    const auto& prod  = lt.productions.at(action.production_id);
                    std::stringstream ss;
                    production_printer(ss, prod);

                    adata["type"] = "reduce";
                    adata["prodid"] = int(action.production_id);

                    adata["production"] = ss.str();
                    adata["count"] = prod.items.size();
                    if (prod.items.empty()) {
                        adata["returnlevels"] = 0;
                    } else {
                        adata["returnlevels"] = prod.items.size() - 1;
                    }

                    adata["symbol"] = "TOK_" + std::string(prod.rule.token_name()) ;
                    adata["valuetype"] = std::string(prod.rule.get_data<symbol_type::rule>()->type_str);
                    if (adata["valuetype"] != "void") { 
                        adata["hasvaluetype"] = "Y";
                    } else {
                        adata["hasvaluetype"] = "N";
                    }

                    adata["hassemaction"] = (prod.action == "" ? "N" : "Y");
                }
                break;
            default :
                yfail("action_type out of range");
                break;
        }

        actions_data.push_back(adata);
    }

    sdata["actions"] = actions_data;

    auto gotos_data = json::array();
    for (const auto& g_iter : state.gotos) {
        auto gdata = json::object();
        gdata["symbol"] = "TOK_" + std::string(g_iter.first.token_name());
        gdata["stateid"] = int(g_iter.second);
        gotos_data.push_back(gdata);
    }
    sdata["gotos"] = gotos_data;

    return sdata;
}

json generate_reduce_functions(const lrtable& lt) {

    auto funcs = json::array();

    for (const auto & [_, prod]  : lt.productions ) {
        if (prod.action != "") {
            auto rdata = json::object();
            rdata["prodid"] = int(prod.prod_id);
            auto item_types = json::array();
            int index = prod.items.size()+1;
            for (const auto &i : util::reverse(prod.items)) {
                auto type_data = json::object();
                auto type_sv = i.sym.do_visit( overloaded{
                    [](const terminal_symbol& t) { return t.type_str; },
                    [](const rule_symbol& r) { return r.type_str; },
                    [](const skip_symbol& s) { return ""sv; },
                });
                type_data["type"] = type_sv;
                type_data["index"] = --index;
                type_data["alias"] = (i.alias ? *i.alias : ""sv);

                item_types.push_back(type_data);
            }
            rdata["itemtypes"] = item_types;
            rdata["block"] = prod.action;

            std::stringstream ss;
            production_printer(ss, prod);

            rdata["production"] = ss.str();
            auto rule_data = prod.rule.get_data<symbol_type::rule>();
            yassert(rule_data, "Could not get the data pointer for a rule.");
            rdata["rule_type"] = rule_data->type_str;

            funcs.push_back(rdata);
        }
    }
    return funcs;
}

/****************************************************************************/
json generate_verbatim(const lrtable& lt) {

    json retval = json::object();

    for (auto v : verbatim_locations) {
        auto pieces = json::array();
        auto range =  lt.verbatim_map.equal_range(std::string(v));

        for (auto t = range.first; t != range.second; ++t) {
            pieces.push_back(std::string(t->second));
        }
        auto loc = std::string(v);
        loc.replace(loc.find('.'), 1, "_");
        retval[loc] = pieces;
    }

    return retval;
}

/****************************************************************************/
void generate_code(const lrtable& lt, std::ostream& outstrm) {

    inja::Environment env;
    env.set_expression("<%", "%>");

    json data;
    data["namespace"] = std::string(lt.options.code_namespace.get());
    data["parserclass"] = std::string(lt.options.parser_class.get());
    data["lexerclass"] = std::string(lt.options.lexer_class.get());


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

    for (const auto &[_, sym] : lt.symbols) {
        std::string tok_name = "TOK_" + std::string(sym.token_name());
        if (sym.isterm()) {
            // terms go in the enum and the term list
            // type names go in the set
            if (sym.name() == "$") {
                enum_entries.push_back(json::object({ 
                        { "name" , "eoi"}, {"value", int(sym.id()) } }));
            } else {
                enum_entries.push_back(json::object({ 
                        { "name" , tok_name }, {"value", int(sym.id()) } }));
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
                    { "name" , tok_name }, {"value", int(sym.id()) } }));
        } else if (sym.isskip()) {
            // Skips only go in the term list
            terms.push_back(sym);
        } else {
            yfail("Unknown symobl type");
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

        std::string pattern;
        case_type ct;
        pattern_type pt;
        const auto* info_ptr = sym.get_data<symbol_type::terminal>();

        if (info_ptr == nullptr) {
            const auto *skip_ptr = sym.get_data<symbol_type::skip>();
            pattern = std::string(skip_ptr->pattern);
            pt = skip_ptr->pat_type;
            ct = skip_ptr->case_match;
        } else {
            pattern =  std::string(info_ptr->pattern);
            pt = info_ptr->pat_type;
            ct = info_ptr->case_match;
        }

        tdata["flags"] = " ";
        if (pt == pattern_type::string) {
            if (ct == case_type::fold) {
                tdata["matcher"] = "fold_string_matcher";
            } else {
                tdata["matcher"] = "string_matcher";
            }
            tdata["pattern"] = "R\"%_^xx(" + pattern + ")%_^xx\"" ;
        } else {
            tdata["matcher"] = "regex_matcher";
            tdata["pattern"] = "R\"%_^xx(" + pattern  + ")%_^xx\"" ;
            if (ct == case_type::fold) {
                tdata["flags"] = ", std::regex::icase";
            }
        }

        if (sym.isterm()) {
            tdata["token"] = "TOK_" + std::string(info_ptr->token_name);
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

    data["reducefuncs"] = generate_reduce_functions(lt);

    data["verbatim"] = generate_verbatim(lt);

/*    std::cout << "----------------------------\n";
    std::cout << data.dump(4) << std::endl;
    std::cout << "----------------------------\n";
*/
    // write the template
    auto main_templ = env.parse(yalr::codegen::main_template);
    env.render_to(outstrm, main_templ, data);

    if (lt.options.code_main.get() == true) {
        outstrm << yalr::codegen::gen_main_code;
    }

}

} //namespace yalr
