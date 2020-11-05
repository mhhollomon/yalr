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


struct dfa_transition_info_t {
    int state_id;
    char low;
    char high;
    int next_state;
};

constexpr std::array<dfa_transition_info_t const, <%dfa.trans_count+1%>> dfa_transitions = {{
## for trans in dfa.transitions
    { <%trans.id%>, <%trans.input_low%>, <%trans.input_high%>, <%trans.next_state%> },
## endfor
    { -9999, '\0', '\0', 0 },
}};

struct dfa_token_info_t {
    int state_id;
    token_type accepted;
    int rank;
};

constexpr std::array<dfa_token_info_t const, <%dfa.token_count+1%>> dfa_token_info = {{
## for symbol in dfa.tokens
    { <%symbol.id%>, <%symbol.token_name%>, <%symbol.rank%> },
## endfor
    { -9999, token_type::undef, 0 },
}};

struct dfa_state_info_t {
    int state_id;
    int tokens;
    int transitions;
};

constexpr int dfa_start_state = <%dfa.start_state%>;
constexpr std::array<dfa_state_info_t const, <%dfa.state_count%>> dfa_state_info = {{
## for state in dfa.state
    { <%state.id%>, <%state.accepted%>, <%state.offset%>},
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

    static inline const std::array<std::tuple<match_ptr, token_type, int>, <%pattern_count%>> patterns = {{
## for pat in patterns
        { std::make_shared<<%pat.matcher%>>( <%pat.pattern%> <%pat.flags%> ), <%pat.token%>, <%pat.rank%> },
##endfor
    }};

    //******************************************************************************************
    // DFA Matching algorithm
    //******************************************************************************************
    //
    struct dfa_match_results {
        token_type tt = token_type::undef;
        int rank = -1;
        int length = 0;
    };

    //**********************
    // Find a transition for the given state and input.
    // Return new state if found. Return -1 if not.
    //**********************
    int find_transition(int state_id, char input) {

        auto offset = dfa_state_info[state_id].transitions;

        // This state has no transitions
        if (offset < 0) { return -1; }

        auto ptr = &dfa_transitions[offset];

        while (ptr->state_id == state_id) {
            if (ptr->low <= input and ptr->high >= input) {
                return ptr->next_state;
            }
            ++ptr;
        }

        return -1;
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
                last_match.length += 1;
                auto const token_offset = dfa_state_info[new_state].tokens;
                if (token_offset >= 0) {
                    auto const *token_ptr = &dfa_token_info[token_offset];
                    while (token_ptr->state_id == new_state) {
                        auto tt = token_ptr->accepted;
                        YALR_LDEBUG( "dfa: new state " << new_state << " is accepting tt = " << tt << "\n");
                        if (tt == token_type::skip or not allowed_tokens or allowed_tokens->count(tt)) {
                            YALR_LDEBUG( "dfa: which IS allowed\n");
                            last_match.tt = tt;
                            last_match.rank = token_ptr->rank;
                            break;
                        } else {
                            YALR_LDEBUG( "dfa: but that isn't allowed\n");
                            ++token_ptr;
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
        int current_match_rank = -1;
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
            current_match_rank = dfa_res.rank;
            YALR_LDEBUG( "lexer: dfa matched for tt = " << ret_type << " length = " << max_len << "\n");
        } else {
            YALR_LDEBUG( "lexer: no dfa match\n");
        }

        for (const auto &[m, tt, rank] : patterns) {
            // if there is a token restriction and we're not on the include list,
            // skip.
            if (allowed_tokens and allowed_tokens->count(tt) == 0) {
                continue;
            }
            YALR_LDEBUG("lexer: Matching for token # " << tt << " rank = " << rank << "\n");
            auto [matched, len] = m->try_match(current, last);
            if (matched) {
                YALR_LDEBUG(" length = " << len << "\n");
                // Override for the same length match if the single matchers came earlier than the dfa match
                if ( std::size_t(len) > max_len or (std::size_t(len) == max_len and rank < current_match_rank)) {
                    max_len = len;
                    ret_type = tt;
                    current_match_rank = rank;
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
