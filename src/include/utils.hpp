#if ! defined(YALR_UTILS_HPP)
#define YALR_UTILS_HPP

#include <iostream>
#include <string>
#include <sstream>

namespace yalr::util {

    template <typename Derived>
    class identifier_t;

    template<typename Derived>
    std::ostream& operator<<(std::ostream& strm, const identifier_t<Derived>& o);


    //
    // identifier_t
    //
    // A simple blind key identifier. Meant to be used to
    // derive a new type that is guaranteed not to interchange
    // with other types:
    //
    // struct my_id : public identifier_t<my_id> { using identifier_t<my_id>::identifier_t; };
    // 
    template <typename Derived>
    class identifier_t {
        static inline int next_value = -1;
        int id = -1;

        identifier_t(int t) : id(t) {}

      public:

        using tag_t = Derived;

        static tag_t get_next_id() {
            return tag_t(++next_value);
        }

        // default
        identifier_t() = default;

        // copy
        identifier_t(const identifier_t &o) = default;
        identifier_t &operator=(const identifier_t &o) = default;

        // move
        identifier_t(identifier_t &&o) = default;
        identifier_t &operator=(identifier_t &&o) = default;

        explicit operator int() const { return id; }

        bool operator ==(const identifier_t& o) const {
            return (o.id == id);
        }
        bool operator !=(const identifier_t& o) const {
            return (o.id != id);
        }
        bool operator <(const identifier_t& o) const {
            return (id < o.id);
        }
        bool operator >(const identifier_t& o) const {
            return (id > o.id);
        }

        friend std::ostream& operator<< <>(std::ostream& strm, const identifier_t& o);
    };

    template<typename Derived>
    std::ostream& operator<<(std::ostream& strm, const identifier_t<Derived>& o) {
        strm << o.id;
        return strm;
    }


    //
    // Adapter to reverse an iterable
    //
    // Cribbed from stackoverflow : 
    //   https://stackoverflow.com/a/28139075/10741691
    //
    template <typename T>
    struct reversion_wrapper { T& iterable; };

    template <typename T>
    auto begin (reversion_wrapper<T> w) { return std::rbegin(w.iterable); }

    template <typename T>
    auto end (reversion_wrapper<T> w) { return std::rend(w.iterable); }

    template <typename T>
    reversion_wrapper<T> reverse (T&& iterable) { return { iterable }; }



    //
    // concat
    //
    // concatentate any number of arguments
    // using their operator<<().
    //
    template<typename ...Args>
    std::string concat(Args&&... args) {
        std::stringstream ss;

        (ss << ... << args);

        return ss.str();

    }

    // escape_char
    //
    // create string that escapes a character if used inside
    // single quotes in C++ source code.
    //
    std::string escape_char(char c, bool use_char_name = false);

    std::string escape_string(std::string_view v, bool use_char_name = false);

} // namespace yalr::util

#endif
