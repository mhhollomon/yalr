# Yet Another LR Parser Generator

C++ will be generated.

Targetting LALR at first but GLR as the end goal.

The generated parser will use [recursive ascent](https://en.wikipedia.org/wiki/Recursive_ascent_parser) (_is this possible for GLR with its split stack?_)

## Status

Current sub-goal is to have yalr generate a language recognizer - that is, the
generated code will simply give a yes/no answer to the question "does the input
string match the grammar?"

- Specification parser - complete for the limited features set.
- Syntax analyzer - complete for the limited feature set.
- LR Parser Table generator - complete. - SLR(0) for the moment. Hard codes
  reduce as priority over shift in shift/reduce conflicts. Fails on
  reduce/reduce conflicts.
- Code generator - complete.
- lexer - not started. Haven't finalized an approach. May use regex for the
  short term.

After the sub-goal is complete, I will probably stop to tidy up the place - add
unit tests, make the parser a bit better about error reporting, etc.

## References
- [Elkhound](http://scottmcpeak.com/elkhound/sources/elkhound/index.html)
- [Lemon](http://www.hwaci.com/sw/lemon/)
- [LR on Wikipedia](https://en.wikipedia.org/wiki/LR_parser)
- [GLR on Wikipedia](https://en.wikipedia.org/wiki/GLR_parser)
- [Boost::Spirit::X3](https://www.boost.org/doc/libs/develop/libs/spirit/doc/x3/html/index.html)
