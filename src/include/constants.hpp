#if not defined(YALR_CONSTANTS_HPP_)
#define YALR_CONSTANTS_HPP_


namespace yalr {

    // 
    // Association types
    //
    enum class assoc_type {
        undef, left, right
    };

    //
    // Case match behavior types
    //
    enum class case_type {
        undef, match, fold
    };

    //
    // Types of patterns in terminals
    //
    enum class pattern_type {
        undef, string, regex
    };

    //
    // Actions in the lrtable
    //
    enum class action_type {
        invalid = -1,
        shift, reduce, accept
    };

} // end namespace yalr

#endif
