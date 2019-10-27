#if not defined(YALR_OPTIONS_HPP)
#define YALR_OPTIONS_HPP

#include "sourcetext.hpp"
#include <string_view>
#include <map>
#include <cassert>

namespace yalr {

using namespace std::literals::string_literals;
using namespace std::literals::string_view_literals;

struct default_setting {
    std::string_view option;
    std::string_view value;
};

struct option_setting {
    std::string_view option;
    std::string_view default_value;
    optional_text_fragment value;

    option_setting(const default_setting & ds) : option(ds.option),
        default_value(ds.value), value(std::nullopt) {}

    std::string_view get_value() const {
        return value ? value->text : default_value;
    };

    optional_text_fragment get_fragment() const { return value; }
    bool is_default() const { return not bool(value); }
};


class option_list {
    std::map<std::string_view, option_setting> settings;

  public:

    // T better be something that can be used in a range and contain default_settings
    // eg. std::vector<default_setting> or default_setting[] or the like.
    // Not sure how to constrain that.
    template<typename T> 
    void init_defaults(T &defaults) {
        for (const auto &i : defaults) {
            settings.emplace(i.option, option_setting(i));
        }
    }

    bool contains(std::string_view opt) const {
        return (settings.count(opt) > 0);
    }

    //
    // Note that only the program should be checking this, so
    // if the setting doesn't exist it is a program error.
    //
    const option_setting &get_setting(std::string_view opt) const {
        auto iter = settings.find(opt);
        assert(iter != settings.end());
        return iter->second;
    };

    bool set_option(std::string_view option, text_fragment value, bool allow_override = false) {
        auto iter = settings.find(option);
        assert(iter != settings.end());

        auto &os = iter->second;

        if (os.is_default() or allow_override) {
            os.value = value;
            return true;
        } else {
            return false;
        }
    }

};
    

struct options {
    constexpr static inline auto PARSER_CLASS = "parser.class"sv;
    constexpr static inline auto LEXER_CASE   = "lexer.case"sv;
    constexpr static inline auto LEXER_CLASS  = "lexer.class"sv;
    constexpr static inline auto NAMESPACE    = "namespace"sv;
};

} // namespace yalr

#endif
