#if not defined(YALR_SYMBOLS_HPP)
#define YALR_SYMBOLS_HPP

#include "parse_tree.hpp"
#include "utils.hpp"
#include "overload.hpp"

#include "constants.hpp"

#include <set>


namespace yalr {

struct symbol_identifier_t : public util::identifier_t<symbol_identifier_t> {
    using util::identifier_t<symbol_identifier_t>::identifier_t;
};


struct terminal_symbol {
    symbol_identifier_t id;
    bool                is_inline = false;
    std::string_view    name;
    std::string_view    type_str;
    std::string_view    pattern;
    std::string_view    token_name;
    std::string_view    action;
    // These will need to be computed in the analyzer
    assoc_type          associativity = assoc_type::undef;
    std::optional<int>  precedence = std::nullopt;
    case_type           case_match = case_type::undef;
    pattern_type        pat_type = pattern_type::undef;

    terminal_symbol(const terminal_stmt& t) :
        name(t.name.text), type_str(t.type_str ? t.type_str->text : "void"sv),
        pattern(t.pattern.text), action(t.action ? t.action->text : ""sv)
        {}
    terminal_symbol() = default;
};

struct skip_symbol {
    symbol_identifier_t id;
    std::string_view    name;
    std::string_view    pattern;
    std::string_view    token_name;
    // These will need to be computed in the analyzer
    case_type           case_match = case_type::undef;
    pattern_type        pat_type = pattern_type::undef;

    skip_symbol(const skip_stmt& s): 
        name(s.name.text), 
        pattern(s.pattern.text) {}
    skip_symbol() = default;
};

struct rule_symbol {
    symbol_identifier_t id;
    std::string_view    name;
    std::string_view    type_str;
    bool                isgoal;

    rule_symbol(const rule_stmt& r) : name(r.name.text),
        type_str(r.type_str? (*r.type_str).text : "void"sv),
        isgoal(r.isgoal) { }
    rule_symbol() = default;

};


enum class symbol_type {
    invalid = -1,
    terminal, skip, rule
};

using symbol_data = std::variant<terminal_symbol, skip_symbol, rule_symbol>; 

class symbol {
    //
    // The idea is to keep symbol as small - and therefore cheap to copy -
    // as possible. So, most of the data is in symbol_data and here
    // we have just enough to allow us to do our job.
    //
    symbol_identifier_t m_id; // repeated here to save the indirection
    std::shared_ptr<symbol_data> data;

    constexpr inline static std::string_view type_names[] = {
        "terminal"sv,
        "skip"sv,
        "rule"sv };

    symbol(terminal_symbol t) : m_id(t.id),
        data(std::make_shared<symbol_data>(t)) {}
    symbol(skip_symbol t) : m_id(t.id),
        data(std::make_shared<symbol_data>(t)) {}
    symbol(rule_symbol t) : m_id(t.id),
        data(std::make_shared<symbol_data>(t)) {}

    friend class symbol_table;

  public:
    symbol_identifier_t  id() const {
       return m_id;
    }

    std::string_view name() const {
        return std::visit( [](auto&& V) { return V.name; }, *data);
    }

    std::string_view token_name() const {
        return std::visit( overloaded {
            [](skip_symbol V) { return V.token_name; },
            [](rule_symbol V) { return V.name; },
            [](terminal_symbol V) { return V.token_name; }
            }, *data);
    }

    bool isskip() const {
        return type() == symbol_type::skip;
    }

    bool isterm() const {
        return type() == symbol_type::terminal;
    }

    bool isrule() const {
        return type() == symbol_type::rule;
    }

    bool operator==(const symbol& o) const {
        return (m_id == o.m_id);
    };

    bool operator<(const symbol& o) const {
        return (m_id < o.m_id);
    };

    symbol_type type() const {
        return symbol_type{int(data->index())};
    };

    std::string_view type_name() const {
        return type_names[int(data->index())];
    };

    template <symbol_type S>
    auto get_data() const {
        return std::get_if<int(S)>(data.get());
    }

    symbol() = default;
    symbol(const symbol&) = default;
    symbol& operator=(const symbol&) = default;
    symbol(symbol&&) = default;
    symbol& operator=(symbol&&) = default;

    template<class T>
    auto do_visit(T visitor) const {
        return std::visit(visitor, *(data.get()));
    }

};

class symbol_table {
    //
    // Look up maps
    //
    std::map<std::string_view, symbol>      key_map;
    std::map<symbol_identifier_t, symbol>   id_map;

  public:
    std::optional<symbol> find(std::string_view key) const {
        auto i = key_map.find(key);
        if (i != key_map.end()) {
            return i->second;
        } else {
            return std::nullopt;
        }
    };

    std::optional<symbol> find(symbol_identifier_t id) const {
        auto i = id_map.find(id);
        if (i != id_map.end()) {
            return i->second;
        } else {
            return std::nullopt;
        }
    };

    auto begin() const { return id_map.begin(); }
    auto end()   const { return id_map.end(); }

    template<typename T>
    std::pair<bool, symbol> add(std::string_view key, T t) {
        t.id = symbol_identifier_t::get_next_id();
        auto [ iter, inserted ] = key_map.try_emplace(key, symbol{t});
        
        if (!inserted) {
            return {false, iter->second};
        }

        auto [id_iter, id_inserted ] = id_map.try_emplace(t.id, iter->second);
        assert(id_inserted);

        return { true, iter->second };
    }


    std::pair<bool, symbol> register_key(std::string_view key, symbol s) {
        auto [ iter, inserted ] = key_map.try_emplace(key, s);
        return {inserted, iter->second };
    }
};

//
// symbol_set
//
// A thin wrapper around std::set to add some helper routines.
//  addset - Add another set to this one and return a bool
//      saying if the set changed.
//  contains - simpler interface than find to see if a symbol is in the set.
//
struct symbol_set : std::set<symbol> {
    //
    // Use our base class constructors.
    //
    using std::set<symbol>::set;

    //
    // add one from a sincle symbol
    //
    symbol_set(const symbol &s) {
        insert(s);
    }

    //
    // addset
    //
    // Add another set to this one.
    // Return true iff the number of elements in the set changed.
    //
    bool addset(const symbol_set& o) {
        auto n = size();
        this->insert(o.begin(), o.end());

        return n != size();
    }

    //
    // contains
    //
    // Check if the set contains the given symbol. Note that C++20
    // will have this natively.
    //
    bool contains(const symbol& s) {
        return (count(s) > 0);
    }
};


} // end namespace yalr
#endif
