#if !defined(YALR_OVERLOAD_HPP)
#define YALR_OVERLOAD_HPP

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

#endif
