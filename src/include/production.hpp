#if not defined(YALR_PRODUCTION_HPP)
#define YALR_PRODUCTION_HPP

#include "utils.hpp"
#include "symbols.hpp"

#include <vector>

namespace yalr {

    struct production_identifier_t : public util::identifier_t<production_identifier_t> {
        using util::identifier_t<production_identifier_t>::identifier_t;
    };

    struct prod_item {
        symbol sym;
        std::optional<std::string_view> alias;
        prod_item(yalr::symbol s, std::optional<std::string_view > sv) :
            sym(s), alias(sv) {}
        prod_item() = default;
        prod_item& operator=(const prod_item&) = default;
    };

    struct production {
        production_identifier_t prod_id;
        symbol rule;
        std::string_view action;
        std::optional<int> precedence;
        std::vector<prod_item> items;

        production(yalr::production_identifier_t id, yalr::symbol s, const std::string_view a, std::vector<yalr::prod_item> i) :
            prod_id(id), rule(s), action(a), items(i) {}

        production() = default;
        production(const production&) = default;
        production(production&&) = default;
        production & operator=(const production &) = default;
        production & operator=(production &&) = default;
    };

} // namespace yalr
#endif
