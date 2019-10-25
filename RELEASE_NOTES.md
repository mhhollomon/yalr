## On Dev

### Functional Changes

- enhancement #20 - Add associativity and precedences markers to terminal and
  rules. See the [README](Readme.md) for details and the calculator example to
  see it in action.

- bug #22 - The examples had not kept up with some of the changes to the
  languages. They have been cleaned up a bit. In particular, the calculator
  example now acts as a good endto-end look at most of the features of the tool

- enhancement #23 - There is now a true install target.

- enhancement #25 - New Precedence and associativity statements. These allow
  you to define terminals and set either precedence or associativity in one
  statement. This keeps you from having to give a name to small operators, etc.

- enhancement #27 - the special type `@lexeme` can now be used in terminals
  and rules. For terminals, it acts as short hand for the common pattern of
  simply returning the parsed text as the semantic value.

- Regex terminal patterns can now be made case insensitive via the `rf:`
  prefix. Fixed strings cannot (yet) be made insensitive.

### Non-functional Changes

- issue #15 - Now using the error_info system to report errors from the
  analyzer. This will allow better error messages in the future.

- Add a set of technical diagrams to the docs/ directory. These were created in
  [Umlet](http://www.umlet.com)

## Release v0.1.2

Bug fix release.

### Functional Changes

- fix for bug #17 - The generated code will not compile if NDEBUG is set.
- implement enhancement #16 - inline namespaces.

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
