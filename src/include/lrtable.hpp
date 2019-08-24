#if not defined(YALR_LRTABLE_HPP)
#define YALR_LRTABLE_HPP

#include "production.hpp"
#include "symbols.hpp"
#include "utils.hpp"


#include <set>
#include <vector>
#include <map>

namespace yalr {

    struct state_identifier_t : public util::identifier_t<state_identifier_t> {
        using util::identifier_t<state_identifier_t>::identifier_t;
    };

    enum class action_type {
        invalid = -1,
        shift, reduce, accept
    };

    struct lr_item {
        production_identifier_t prod_id;
        int position = 0;

        lr_item(production_identifier_t id, int pos) :
            prod_id(id), position(pos) {}

        lr_item() = default;

        bool operator==(const lr_item &o) const {
            return (
                    prod_id == o.prod_id && position == o.position
                   );
        }
        bool operator<(const lr_item& o) const {
            return (
                    prod_id < o.prod_id ? true :
                    ( prod_id == o.prod_id ? position < o.position : false )
                   );
        }
    };

    struct item_set : std::set<lr_item> {
        using std::set<lr_item>::set;
    };

    struct transition {
        symbol on_symbol;
        state_identifier_t new_state_id;

        transition(symbol s, state_identifier_t id) :
            on_symbol(s), new_state_id(id) {}
    };

    struct action {
        action_type type;
        state_identifier_t new_state_id;
        production_identifier_t production_id;

        action(yalr::action_type at) : type(at) {}

        action(yalr::action_type at,
                state_identifier_t sid) :
            type(at), new_state_id(sid) {}

        action(yalr::action_type at,
                production_identifier_t pid) :
            type(at), production_id(pid) {}

        action(yalr::action_type at,
                state_identifier_t sid,
                production_identifier_t pid) :
            type(at), new_state_id(sid),
            production_id(pid) {}

        std::string type_name() const {
            return names[int(type)];
        }

      private:
        static inline std::string names[] = {
            "shift"s, "reduce"s, "accept"s
        };
    };

    struct lrstate {
        state_identifier_t id;
        item_set items;
        bool initial;
        std::map<symbol, transition> transitions;
        std::map<symbol, action> actions;
        std::map<symbol, state_identifier_t> gotos;

        lrstate(item_set is, bool init=false) :
            id(state_identifier_t::get_next_id()),
            items(is), initial(init) {}
    };

    using production_map = std::map<production_identifier_t, production>;
    struct lrtable {
        std::vector<lrstate> states;
        symbol_table symbols;
        production_map productions;
        std::map<symbol, symbol_set> first_set;
        std::map<symbol, symbol_set> follow_set;
        symbol_set epsilon;
        production_identifier_t target_prod;
        option_list options;
    };



} // namespace yalr
#endif
