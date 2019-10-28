#if not defined(YALR_OPTIONS_HPP)
#define YALR_OPTIONS_HPP

//#include "sourcetext.hpp"
#include "constants.hpp"
#include "yassert.hpp"

#include <string_view>
#include <unordered_map>
#include <set>
#include <functional>

namespace yalr {

using namespace std::literals::string_literals;
using namespace std::literals::string_view_literals;


struct _option_table_base {
    std::unordered_map<std::string_view, std::function<bool(std::string_view)>> valid_funcs;

    void _register(std::string_view k, std::function<bool(std::string_view)> f) {
        valid_funcs.emplace(k, f);
    }

    bool validate(const std::string_view opt, std::string_view val) {
        return valid_funcs[opt](val);
    }

    bool contains(std::string_view opt_name) {
        return valid_funcs.count(opt_name) > 0;
    }

};

template <class T, class Derived>
class option  {
    T default_val_;
    T value_;
    bool is_set_ = false;
    bool resettable_ = true;
    std::string_view name_;
  public:

    option() = default;
    option(std::string_view n, Derived& opt, _option_table_base& parent, 
            bool can_set_multiple, T def) :
        default_val_{def}, resettable_(can_set_multiple), name_{n}
         {

        parent._register(n, [&opt](std::string_view val) mutable { return opt.validate(val); });
    }

    T get() const {
        return (is_set_? value_ : default_val_);
    }

    bool set(T val) {
        if (is_set_ and not resettable_) {
            return false;
        }

        value_ = val;
        is_set_ = true;
        return true;
    }

    bool defaulting() const { return !is_set_; }

    friend struct option_table;
};

/***************************************************
 * Option class for string_view value, but can only be
 * set once in a gammar file (e.g. lexer.class)
 ****************************************************/
struct sv_once_option : public option<std::string_view, sv_once_option> {
    sv_once_option(std::string_view v, _option_table_base& parent, std::string_view def) : 
        option{v, *this, parent, false, def} {}

    bool validate(std::string_view val) {
        return set(val);
    };
};

/*********************************************************
 * Option class for bool value which can be set mutiple times
 *********************************************************/
struct bool_option : public option<bool, bool_option> {
    std::set<std::string_view> true_things = { "yes", "YES", "true", "TRUE", "1" };
    bool_option(std::string_view v, _option_table_base& parent, bool def) : 
        option{v, *this, parent, true, def} {}

    bool validate(std::string_view val) {
        if (true_things.count(val) > 0) {
            return set(true);
        } else {
            return set(false);
        } 
    };
};

struct lexer_case_option : public option<case_type, lexer_case_option> {
    lexer_case_option(std::string_view v, _option_table_base& parent, case_type def) : 
        option{v, *this, parent, true, def} {}

    bool validate(std::string_view val) {
        if (val == "cmatch") {
            set(case_type::match);
            return true;
        } else if (val == "cfold") {
            set(case_type::fold);
            return true;
        }

        return false;
    };
};


/*****************************************************************************
 * Option Table
 *****************************************************************************/
struct option_table : public _option_table_base {

    //                             name                      default
    sv_once_option      lexer_class{"lexer.class",    *this, "Lexer"};
    sv_once_option     parser_class{"parser.class",   *this, "Parser"};
    sv_once_option   code_namespace{"code.namespace", *this, "YalrParser"};
    lexer_case_option    lexer_case{"lexer.case",     *this, case_type::match};
    bool_option           code_main{"code.main",      *this, false};

};

} // namespace yalr

#endif
