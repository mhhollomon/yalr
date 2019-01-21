#if ! defined(YALR_SYMBOLTABLE_HPP)
#define YALR_SYMBOLTABLE_HPP

#include "ast.hpp"

#include <set>
#include <string>
#include <map>
#include <list>
#include <cassert>
#include <memory>
#include <type_traits>

namespace yalr {
    class SymbolTable {
    public:
        enum class symbol_type { unknown, rule, term, skip };

    private:
        struct symbol_imp {
            int id;
            symbol_type stype;
            std::unique_ptr<ast::symbol> ast;

            symbol_imp(int i, const ast::rule_def& r) :
                id(i), stype(symbol_type::rule),
                ast(std::make_unique<ast::rule_def>(r)) { }

            symbol_imp(int i, const ast::terminal& r) :
                id(i), stype(symbol_type::term),
                ast(std::make_unique<ast::terminal>(r)) { }

            symbol_imp(int i, const ast::skip& r) :
                id(i), stype(symbol_type::skip),
                ast(std::make_unique<ast::skip>(r)) { }

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
                    case symbol_type::skip:
                        return "Skip";
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
            std::string name() const { return m_sym->ast->name; }
            std::string type_name() const { return m_sym->type_name(); }
            bool isterm() const { return (stype() == symbol_type::term); }
            bool isrule() const { return (stype() == symbol_type::rule); }
            bool isskip() const { return (stype() == symbol_type::skip); }

            const ast::terminal *getTerminalInfo() const {
                return (stype() == symbol_type::term ?
                        // ast::symbol doesn't have a virtual table
                        // so, can't use dynamic_cast.
                        static_cast<ast::terminal *>(m_sym->ast.get())
                        : nullptr);
            }
            const ast::rule_def *getRuleInfo() const {
                return (stype() == symbol_type::rule ? 
                        static_cast<ast::rule_def *>(m_sym->ast.get())
                        : nullptr);
            }

            const ast::skip *getSkipInfo() const {
                return (stype() == symbol_type::skip ? 
                        static_cast<ast::skip *>(m_sym->ast.get())
                        : nullptr);
            }
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
        std::map<std::string, symbol> by_alias;

    public:

        template <class T>
        std::pair<bool, symbol>
        insert(const T& r) {

            auto iter = by_name.find(r.name);

            if (iter != by_name.end()) {
                return std::make_pair(false, iter->second);
            } 

            const auto new_ptr = std::make_shared<const symbol_imp>(++id_gen, r);
            auto new_sym = symbol{new_ptr};
            table.push_back(new_ptr);

            by_name.emplace(r.name, new_sym);
            by_id.emplace(id_gen, new_sym);
            if constexpr(std::is_same<ast::terminal, T>::value) {
                if (r.pattern[0] == '\'') {
                    //std::cerr << "Registering " << r.pattern << "as alias to " << r.name << "\n";
                    by_alias.emplace(r.pattern, new_sym);
                } else {
                    //std::cerr << "Not a single quote pattern " << r.pattern << "\n";
                }
            }

            return std::make_pair(true, new_sym);
        }

        std::pair<bool, symbol> find(const std::string& s) const {
            auto iter = by_name.find(s);
            if (iter != by_name.end()) {
                return std::make_pair(true, iter->second);
            }

            auto iter2 = by_alias.find(s);
            if (iter2 != by_alias.end()) {
                return std::make_pair(true, iter2->second);
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

    struct symbolset : std::set<SymbolTable::symbol> {
        using std::set<SymbolTable::symbol>::set;
        bool addset(const symbolset& o) {
            auto n = size();
            this->insert(o.begin(), o.end());

            return n != size();
        }
        bool contains(const SymbolTable::symbol& s) {
            return (count(s) > 0);
        }
    };

} // end namespace

#endif
