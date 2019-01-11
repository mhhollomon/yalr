#if ! defined(YALR_SYMBOLTABLE_HPP)
#define YALR_SYMBOLTABLE_HPP

#include "ast.hpp"

#include <set>
#include <string>
#include <map>
#include <list>
#include <cassert>
#include <memory>

namespace yalr {
    class SymbolTable {
    public:
        enum class symbol_type { unknown, rule, term };

    private:
        struct symbol_imp {
            int id;
            symbol_type stype;
            std::string name;

            symbol_imp(int i, const ast::rule_def& r) :
                id(i), stype(symbol_type::rule), name(r.name) {}

            symbol_imp(int i, const ast::terminal& r) :
                id(i), stype(symbol_type::term), name(r.name) {}

            bool operator==(const symbol_imp& o) const {
                return ( id == o.id );
            }

            bool operator<(const symbol_imp &o) const {
                return ( id < o.id );
            }

            std::string type_name() const {
                switch(stype) {
                    case symbol_type::unknown:
                        return "Unknown";
                    case symbol_type::rule:
                        return "Rule";
                    case symbol_type::term:
                        return "Term";
                    default:
                        assert(false);
                }

                assert(false);
            }
        };

        using imp_ptr = std::shared_ptr<const symbol_imp>;

    public:
        class symbol {
            int m_id;
            symbol_type m_stype;
            imp_ptr m_sym;

            friend class SymbolTable;
            symbol(const imp_ptr sym) :
                m_id(sym->id), m_stype(sym->stype), m_sym(sym) {}

            static symbol end() {
                return symbol();
            }


        public:

            symbol() : m_id(-9999), m_stype(symbol_type::unknown), m_sym(nullptr) {}
            int id() const { return m_id; }
            symbol_type stype() const { return m_stype; }
            std::string name() const { return m_sym->name; }
            std::string type_name() const { return m_sym->type_name(); }
            bool operator==(const symbol& o) const {
                return (m_id == o.m_id);
            }
            bool operator!=(const symbol& o) const {
                return (m_id != o.m_id);
            }
            bool operator<(const symbol& o) const {
                return (m_id < o.m_id);
            }
        };

    private:
        int id_gen = -1;
        std::list<imp_ptr> table;
        std::map<std::string, symbol> by_name;
        std::map<int, symbol> by_id;

    public:

        std::pair<bool, symbol> insert(const ast::rule_def& r);
        std::pair<bool, symbol> insert(const ast::terminal& t);
        std::pair<bool, symbol> find(const std::string& s) const {
            auto iter = by_name.find(s);
            if (iter != by_name.end()) {
                return std::make_pair(true, iter->second);
            }

            return std::make_pair(false, symbol::end());
        }

        auto begin() const { return by_name.begin(); }
        auto end() const { return by_name.end(); }

        std::pair<bool, symbol> find(int i) const {
            auto iter = by_id.find(i);
            if (iter != by_id.end()) {
                return std::make_pair(true, iter->second);
            }

            return std::make_pair(false, symbol::end());
        }
    };
} // end namespace

#endif
