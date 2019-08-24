## On Master

### Functional Changes
- Add the ability to name the items in a rule alternatives and use those names in rule actions.

### non-functional changes
- Moved to CMake for build. The meson system is currently broken.
- Refactored the code some more.

## Release v0.0.2

### Functional Changes
- term/skip are now specified as either single-quote or `r:` strings.
    - single-quote strings are matched in the lexer as simple string compares
    - `r:` strings are treated as std::regex regular expressions.
- term now takes a type in angle brackets. And can take a semantic action as
  well.
- enhancement #6 - The state table can be dumped using -S option.

### Code improvements
- Moved to [Inja template engine](https://github.com/pantor/inja) to generate
  code.

## Release v0.0.1

First Release. Core Lexer and Parser are functional. The parser is only SLR(1)
at this point. It will be upgraded to LALR(1) in the future.

Currently the generated parser only operates as a recognizer. There is no way
to compute or store semantic values or attributes.
