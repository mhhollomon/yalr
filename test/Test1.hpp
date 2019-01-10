#include <iostream>
namespace Test1 {

enum token_type {
    eoi = 4,
    TOK_A = 0,
    TOK_B = 1,
    TOK_S = 2,
    TOK_S_prime = 3,
    TOK_UNDEF = -1
};


struct Token {
    token_type toktype;
    Token(token_type t) : toktype(t) {}
};

enum state_action { undefined, reduce, accept, error };

struct rettype {
    state_action action;
    int depth;
    int symbol;
    rettype(state_action a = undefined, int d = 0, int s = 0) :
        action(a), depth(d), symbol(s) {}
};

class Lexer {
public:
    virtual Token next_token() {
        /* dummy */;
        return eoi;
    }
};
class Test1 {
    Lexer& lexer;
    rettype state0() {
        std::cout << "entering state 0\n";
        // Reduce
        std::cout << "reducing by : [0] S(2) =>\n";
        return { reduce, 18446744073709551615,2 };
    }

    rettype state1() {
        std::cout << "entering state 1\n";
        // Reduce
        std::cout << "reducing by : [0] S(2) =>\n";
        return { reduce, 18446744073709551615,2 };
    }

    rettype state2() {
        std::cout << "entering state 2\n";

        Token tok = lexer.next_token();

        if (tok.toktype == eoi) {
            // Accept.
            return { accept, 0 };
        } else {
            // ERROR.
            return { error, 0 };
        }
    }

    rettype state3() {
        std::cout << "entering state 3\n";

        Token tok = lexer.next_token();
        rettype retval;

        switch (tok.toktype) {
            case TOK_B : retval = state4(); break;
            default: return { error };
        };

        while (retval.action == reduce) {
            if (retval.depth > 0) {
                retval.depth -= 1;
                return retval;
            }
            switch(retval.symbol) {
            }

        }
        return retval;
    }
    rettype state4() {
        std::cout << "entering state 4\n";
        // Reduce
        std::cout << "reducing by : [1] S(2) => A(0) S(2) B(1)\n";
        return { reduce, 2,2 };
    }

public:
    Test1(Lexer& l) : lexer(l){};

    bool doparse() {
        auto retval = state0();
        if (retval.action == accept) {
            return true;
        }

        return false;
    }
}; // class
} // namespace
