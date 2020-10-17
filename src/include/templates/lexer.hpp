#pragma once

#include <string>

namespace yalr::codegen {

using namespace std::string_literals;

const std::string lexer_template =
R"DELIM(

namespace <%namespace%> {  // for the lexer

template<typename IterType>
class <%lexerclass%> {

public:
    using iter_type = IterType;

private:
    struct matcher {
        virtual std::pair<bool, int>
        try_match(iter_type first, const iter_type last) const = 0;

        virtual ~matcher() {}
    };

    struct string_matcher : matcher {
        std::string pattern;
        string_matcher(std::string p) : pattern{p} {};
        virtual std::pair<bool, int>
        try_match(iter_type first, const iter_type last) const override {
            if (std::size_t(last - first) < pattern.size()) {
                return std::make_pair(false, 0);
            } else if (std::equal(pattern.begin(), pattern.end(), first)) {
                return std::make_pair(true, pattern.size());
            } else {
                return std::make_pair(false, 0);
            }
        }
    };

    struct fold_string_matcher : matcher {
        std::string pattern;
        fold_string_matcher(std::string p) : pattern{p} {};
        virtual std::pair<bool, int>
        try_match(iter_type first, const iter_type last) const override {
            if (std::size_t(last - first) < pattern.size()) {
                return std::make_pair(false, 0);
            } else if (std::equal(pattern.begin(), pattern.end(), first, 
                    [](char cA, char cB) {
                            return toupper(cA) == toupper(cB);
                    })) {
                return std::make_pair(true, pattern.size());
            } else {
                return std::make_pair(false, 0);
            }
        }
    };

    struct regex_matcher : matcher {
        std::regex pattern;
        regex_matcher(std::string p, const std::regex_constants::syntax_option_type& opt) try : pattern{p, opt} {
            {}
        } catch (std::regex_error &e) {
            std::cerr << "Error when compiling pattern '" << p << "'\n";
            throw e;
        }
        regex_matcher(std::string p) try : pattern{p} {
            {}
        } catch (std::regex_error &e) {
            std::cerr << "Error when compiling pattern '" << p << "'\n";
            throw e;
        }
        virtual std::pair<bool, int>
        try_match(iter_type first, const iter_type last) const override {
            std::match_results<iter_type> mr;
            if (std::regex_search(first, last, mr, pattern, 
                    std::regex_constants::match_continuous)) {
                auto len = mr.length(0);
                return std::make_pair(true, len);
            } else {
                return std::make_pair(false, 0);
            }
        }
    };

    using match_ptr = const std::shared_ptr<const matcher>;

    static inline const std::vector<std::tuple<match_ptr, token_type, bool>> patterns = {
## for pat in patterns
        { std::make_shared<<%pat.matcher%>>( <%pat.pattern%> <%pat.flags%> ), <%pat.token%>, <%pat.is_global%> },
##endfor
    };

/***** verbatim lexer.top ********/
## for v in verbatim.lexer_top
<% v %>
## endfor
/***** verbatim lexer.top ********/
public:
#if defined(YALR_DEBUG)
    bool debug = false;
#endif
    Lexer(iter_type first, const iter_type last) :
        current(first), last(last) {
    }

    virtual token next_token( std::optional<std::set<token_type>> allowed_tokens = std::nullopt) {
        if (current == last) {
            YALR_LDEBUG( "Returning token eoi\n");
            return eoi;
        }

        token_type ret_type = undef;
        std::size_t max_len = 0;
#if defined(YALR_DEBUG)
        if (debug) {
            std::cerr << "Next few characters: " ;
            auto ptr = current;
            for (int i =0 ; i < 10 ; ++i) {
                std::cerr << *ptr;
                ++ptr;
            }
            std::cerr << "\n";
        }
#endif

        for (const auto &[m, tt, glbl] : patterns) {
            if (not (glbl or !allowed_tokens or (allowed_tokens and allowed_tokens->count(tt) > 0))) {
                continue;
            }
            YALR_LDEBUG("Matching for token # " << tt << (glbl? " (global)" : ""));
            auto [matched, len] = m->try_match(current, last);
            if (matched) {
                YALR_LDEBUG(" length = " << len << "\n");
                if ( std::size_t(len) > max_len) {
                    max_len = len;
                    ret_type = tt;
                }
            } else {
                YALR_LDEBUG(" - no match\n");
            }
        }
        if (max_len == 0) {
            current = last;
            return token{undef};
        } else if (ret_type == skip) {
            YALR_LDEBUG("recursing due to skip\n");
            current += max_len;
            return next_token(allowed_tokens);
        }
        std::string lx{current, current+max_len};
        semantic_value ret_sval;
        switch (ret_type) {
## for sa in semantic_actions 
            case <%sa.token%> : {
                    auto block = [](std::string&& lexeme) -> <%sa.type%>
                    {  <%sa.block%> };
                    ret_sval = block(std::move(lx));
                }
                break;
## endfor
            default :
                /* do nothing */
                break;
        }

        if (debug) {
            std::string lx{current, current+max_len};
            YALR_LDEBUG( "Returning token = " << ret_type);
            if (ret_type >= 0) {
                YALR_LDEBUG(" (" << token_name[ret_type] << ")")
            }
            YALR_LDEBUG (" lex={" << lx << "}\n");
        }
        current += max_len;
        return token{ret_type, ret_sval};
    }

    // Just needed to make it virtual
    virtual ~Lexer() = default;
private:
    std::string::const_iterator current;
    const std::string::const_iterator last;

/***** verbatim lexer.bottom ********/
## for v in verbatim.lexer_bottom
<% v %>
## endfor
/***** verbatim lexer.bottom ********/
};

} // namespace <%namespace%>


)DELIM"s;



} // namespace yalr::codegen
