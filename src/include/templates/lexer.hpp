#pragma once

#include <string>

namespace yalr::codegen {

using namespace std::string_literals;

const std::string lexer_template =
R"DELIM(

namespace <%namespace%> {  // for the lexer

std::set<token_type> const global_patterns = {
## for pat in patterns
## if pat.is_global == "true" or pat.token == "skip"
    <%pat.token%>, 
## endif
## endfor
};

struct dfa_state_info_t {
    int state_id;
    token_type accepted;
};

constexpr int dfa_start_state = <%dfa.start_state%>;
constexpr std::array<dfa_state_info_t const, <%dfa.state_count%>> dfa_state_info = {{
## for state in dfa.state
    { <%state.id%>, <%state.accepted%> },
## endfor
}};


struct dfa_transition_info_t {
    int state_id;
    char input;
    int next_state;
};

constexpr std::array<dfa_transition_info_t const, <%dfa.trans_count%>> dfa_transitions = {{
## for trans in dfa.transitions
    { <%trans.id%>, <%trans.input%>, <%trans.next_state%> },
## endfor
}};


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

    static inline const std::array<std::tuple<match_ptr, token_type, bool>, <%pattern_count%>> patterns = {{
## for pat in patterns
        { std::make_shared<<%pat.matcher%>>( <%pat.pattern%> <%pat.flags%> ), <%pat.token%>, <%pat.is_global%> },
##endfor
    }};

    //******************************************************************************************
    // DFA Matching algorithm
    //******************************************************************************************
    //
    struct dfa_match_results {
        token_type tt = token_type::undef;
        int length = 0;
    };

    //**********************
    // Find a transition for the given state and input.
    // Return new state if found. Return -1 if not.
    //**********************
    int find_transition(int state_id, char input) {

        auto res = std::lower_bound(dfa_transitions.begin(), dfa_transitions.end(), 
                dfa_transition_info_t{ state_id, input, 0 }, 
                [](const dfa_transition_info_t lhs, dfa_transition_info_t rhs)->bool {
                    return ((lhs.state_id < rhs.state_id) || (lhs.state_id == rhs.state_id 
                                and lhs.input < rhs.input));
                    });

        if (res != dfa_transitions.end() and (res->state_id == state_id and res->input == input)) {
            return res->next_state;
        } else {
            return -1;
        }
    }

    // *****************************
    // ret_val.tt == undef if nothing was matched
    // retval.length will be the number of chars we looked at even 
    // for the no match state.
    //******************************
    dfa_match_results dfa_match(iter_type first, const iter_type last, 
            std::optional<std::set<token_type>> allowed_tokens) {
        dfa_match_results last_match;
        int current_state = dfa_start_state;

        while(true) {
            if (first == last) {
                YALR_LDEBUG("dfa : at eoi - bailing out\n");
                YALR_LDEBUG("dfa : returning {" << last_match.tt << ", " << last_match.length << "}\n");
                return last_match;
            }

            char current_input = *first;
            ++first;

            YALR_LDEBUG( "dfa : current_state = " << current_state <<
                " current_input = '" << current_input << "'\n");
            int new_state = find_transition(current_state, current_input);
            if (new_state == -1) {
                YALR_LDEBUG("dfa : couldn't find a transition\n");
                YALR_LDEBUG("dfa : returning {" << last_match.tt << ", " << last_match.length << "}\n");
                return last_match;
            } else {
                auto iter = std::lower_bound(dfa_state_info.begin(), dfa_state_info.end(),
                        dfa_state_info_t{ new_state, token_type::undef },
                        [](const dfa_state_info_t lhs, dfa_state_info_t rhs) -> bool {
                            return (lhs.state_id < rhs.state_id);
                        } );
                if (iter != dfa_state_info.end()) {
                    last_match.length += 1;
                    while ( true) {
                        if (iter->state_id == new_state and iter->state_id != token_type::undef) {
                            YALR_LDEBUG( "dfa: new state " << new_state << " is accepting tt = " 
                                        << iter->accepted << "\n");
                            if (not allowed_tokens or allowed_tokens->count(iter->accepted) > 0) {
                                last_match.tt = iter->accepted;
                                break;
                            } else {
                                YALR_LDEBUG( "dfa: but that isn't allowed\n");
                                ++iter;
                            }
                        } else {
                            YALR_LDEBUG( "dfa: new state " << new_state << " no allowed tokens\n");
                            break;
                        }
                    }
                        
                } else {
                    YALR_LDEBUG("dfa: new state " << new_state << " is NOT accepting\n");
                }
            }
            current_state = new_state;
        }

    }

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

        if (allowed_tokens) {
            allowed_tokens->insert(global_patterns.begin(), global_patterns.end());
        }

        token_type ret_type = undef;
        std::size_t max_len = 0;
#if defined(YALR_DEBUG)
        if (debug) {
            std::cerr << "lexer: Next few characters: " ;
            auto ptr = current;
            for (int i =0 ; i < 10 && ptr != last; ++i) {
                std::cerr << *ptr;
                ++ptr;
            }
            std::cerr << "\n";

            std::cerr << "lexer: Trying the dfa\n";
        }

#endif

        auto dfa_res = dfa_match(current, last, allowed_tokens);

        if (dfa_res.tt != token_type::undef) {
            ret_type = dfa_res.tt;
            max_len = dfa_res.length;
            YALR_LDEBUG( "lexer: matched for tt = " << ret_type << " length = " << max_len << "\n");
        } else {
            YALR_LDEBUG( "lexer: no dfa match\n");
        }

        for (const auto &[m, tt, glbl] : patterns) {
            // if there is a token restriction and we're not on the include list,
            // skip.
            if (allowed_tokens and allowed_tokens->count(tt) == 0) {
                continue;
            }
            YALR_LDEBUG("lexer: Matching for token # " << tt << (glbl? " (global)" : ""));
            auto [matched, len] = m->try_match(current, last);
            if (matched) {
                YALR_LDEBUG(" length = " << len << "\n");
                // Override for the same length match if the single matchers came earlier than the dfa match
                if ( std::size_t(len) > max_len or tt < ret_type) {
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
            YALR_LDEBUG("lexer: recursing due to skip\n");
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
            YALR_LDEBUG( "lexer: Returning token = " << ret_type);
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
    iter_type current;
    const iter_type last;

/***** verbatim lexer.bottom ********/
## for v in verbatim.lexer_bottom
<% v %>
## endfor
/***** verbatim lexer.bottom ********/
};

} // namespace <%namespace%>


)DELIM"s;



} // namespace yalr::codegen
