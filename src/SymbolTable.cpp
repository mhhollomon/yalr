#include "SymbolTable.hpp"

namespace yalr {

    // Maybe make these templates?
    std::pair<bool, SymbolTable::symbol> 
    SymbolTable::insert(const ast::rule_def& r) {

        auto iter = by_name.find(r.name);

        if (iter != by_name.end()) {
            return std::make_pair(false, iter->second);
        } 

        const auto new_ptr = std::make_shared<const symbol_imp>(++id_gen, r);
        auto new_sym = symbol{new_ptr};
        table.push_back(new_ptr);

        by_name.emplace(r.name, new_sym);
        by_id.emplace(id_gen, new_sym);

        return std::make_pair(true, new_sym);

    }

    std::pair<bool, SymbolTable::symbol> 
    SymbolTable::insert(const ast::terminal& r) {

        auto iter = by_name.find(r.name);

        if (iter != by_name.end()) {
            return std::make_pair(false, iter->second);
        } 

        const auto new_ptr = std::make_shared<const symbol_imp>(++id_gen, r);
        auto new_sym = symbol{new_ptr};
        table.push_back(new_ptr);

        by_name.emplace(r.name, new_sym);
        by_id.emplace(id_gen, new_sym);

        return std::make_pair(true, new_sym);

    }

}
