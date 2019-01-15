#if ! defined(YALR_TABLEGEN_HPP)
#define YALR_TABLEGEN_HPP

#include "analyzer.hpp"

namespace yalr { namespace tablegen {

    using symbol = SymbolTable::symbol;

    struct item {
        int prod_id;
        int position;

        item (int pid = 0, int pos = 0) : prod_id(pid), position(pos) {}

        bool operator==(const item &o) const {
            return (
                    prod_id == o.prod_id && position == o.position
                   );
        }
        bool operator<(const item& o) const {
            return (
                    prod_id < o.prod_id ? true :
                    ( prod_id == o.prod_id ? position < o.position : false )
                   );
        }
    };

    using item_set = std::set<item>;

    struct transition {
        symbol sym;
        int state_id;
        transition(symbol s, int id) : sym(s), state_id(id) {}
    };

    enum class action_type { unknown, shift, reduce, accept };
    struct action {
        action_type atype;
        int new_state_id;
        int prod_id;

        action (action_type t, int s, int p) :
            atype(t), new_state_id(s), prod_id(p) {}

        std::string type_name() const {
            switch (atype) {
                case action_type::unknown :
                    return "unknown";
                case action_type::shift :
                    return "shift";
                case action_type::reduce :
                    return "reduce";
                case action_type::accept :
                    return "accept";
                default :
                    assert(false);
            }
        }
    };

    struct lrstate {
        int id;
        item_set items;
        bool initial = false;
        std::map<symbol, transition>transitions;
        std::map<symbol, action> actions;
        std::map<symbol, int> gotos;

        lrstate(int id, item_set i, bool init = false) :
            id(id), items(i), initial(init) {}
    };


    struct lrtable {
        std::vector<lrstate> states;
        SymbolTable syms;
        std::vector<analyzer::production>productions;
        std::map<symbol, symbolset>firstset;
        std::map<symbol, symbolset>followset;
        symbolset epsilon; /* those symbols that have epsilon productions */
        int target_prod_id;
        std::string parser_class;
    };


    std::unique_ptr<lrtable> generate_table(const analyzer::grammar& g);

    
    void pretty_print(const lrtable& lt, std::ostream& strm);

}}

#endif
