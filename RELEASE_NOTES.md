## Release v0.1.2

Bug fix release.
### Functional Changes

- fix for bug #17 - The generated code will not compile if NDEBUG is set.
- implement enhancement #16 - inline namespaces.

### Non-functional changes

## Release v0.1.0

Yalr is now a function parser generator - complete with semantic actions.

There is still much to be done, but the system can actually do useful work.

### Functional Changes
- Add the ability to name the items in rule alternatives and use those names in rule actions.
- Simple terminals can now be defined "inline". This allows the use of
  single-quoted strings directly in rules. The terminal is added for you by the
  system.

### non-functional changes
- Moved to CMake for build. The meson system is currently broken.
- Add many more tests.
- Refactored the code some more.
- Started adding technical documentation.

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
