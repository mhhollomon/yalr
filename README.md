# Yet Another LR Parser Generator
[![Github Releases](https://img.shields.io/github/release/mhhollomon/yalr.svg)](https://github.com/mhhollomon/yalr/releases)
[![Build Status](https://api.cirrus-ci.com/github/mhhollomon/yalr.svg)](https://cirrus-ci.com/github/mhhollomon/yalr)
[![Github Issues](https://img.shields.io/github/issues/mhhollomon/yalr.svg)](http://github.com/pantor/mhhollomon/yalr)
[![GitHub License](https://img.shields.io/badge/license-MIT-blue.svg)](https://raw.githubusercontent.com/mhhollomon/yalr/master/LICENSE)

C++ will be generated.

Targetting LALR at first but GLR as the end goal.

The generated parser will use [recursive ascent](https://en.wikipedia.org/wiki/Recursive_ascent_parser) (_is this possible for GLR with its split stack?_)

## Status

Yalr currently generates a *language recognizer* - that is, the generated code
will simply give a yes/no answer to the question "Does the input string match
the grammar?"

Both a lexer and a parser are generated.

The next goal is to "tidy up" - add more unit tests, make the parser a
bit better about error reporting, etc.

## Building

Yalr requires boost, meson, ninja, and a c++17 compliant compiler. Assuming you have those, then building is:
```bash
git clone https://github.com/mhhollomon/yalr
cd yalr
mkdir build
cd build
meson ../src
ninja
```

The new executable will be in the build directory.

## Running

At the simplest, it is:

```
yalr my_grammar.yalr
```

This will generate a file `YalrParser.hpp` that has the generated parser in it.

Other options:

```
# get help
yalr -h
yalr --help

# Rename the output file
yalr -o foo.hpp my_grammar.yalr

# Instead of outputting the parser,
# translate the grammer for use on grammophone
# (see references)
yalr -t grammophone my_grammar.yalr
```

## Grammar Spec

Whitespace is generally not significant. `C` style `/* ... */` comments 
as well as C++ `//` comments are supported.

Keywords are reserved and may not be used as the name of a terminal or rule.

The [example
directory](https://github.com/mhhollomon/yalr/tree/master/examples) contains
some example grammars including the grammar for the yalr grammar itself.

### Parser Class Name

The parser is normally put in `class YalrParser`. This can be changed by using the statement:
```
parser class MyClass;
```
This *also* changes the default name for the output to `MyClass.hpp`. 
This can be overriden by the `--output-file` option on the command line.

This statement may only appear once in the file.

It must be the first statement in the grammar.

### Terminals

All terminals must be explicitly declared.

There are two types of terminal - "parser" terminals and "lexer" terminals.

#### Parser Terminals

Parser Terminals are those terminals that are used to create the rules in
grammar. These are the terminals that are return by the lexer.

Parser Terminals are defined by the `term` keyword.

```
// term <ID> <pattern> ;
term MYTERM r:my[0-9] ;
term SEMI ';' ;
term KEYWORD 'keyword';
```

The `ID` is the name for the terminal that will be used in the grammar and will
be returned in error messages. It will also be a part of the enumeration
constant for the token type in the generated code.

The pattern can be specified two different ways.

1. As a single-quote delimited string.
Patterns in this format are matched in the lexer as simple string compares.
The pattern can be used as an alias for the term in rules.

2. std::regex regular expression.
The starting delimiter is the literal `r:`. The pattern extends to the next
unescape space-like character. Note, this means that the semi-colon at the end
of the definition must be separated from the pattern by at least one space (or
tab, or newline).

A parser terminal may also have a type associated with it. If it does, then a
semantic value will be generated and passed to the parser. The type is given in
angle brackets afte the keyword term :

```
term <int> INTEGER r:[-+]?[0-9]+ ;
```

The computation is given as an action encased in `<%{ ... }%>` . If an action
is given, then the normal terminating semi-colon is not required.

```
term <int> INTEGER r:[-+]?[0-9]+ <%{ return std::stoi(lexeme); }%>
```

The action should be though of as the body of a lambda that returns the
semantic value to be passed back to the parser. The identifier `lexeme` is
available. It is a string that contains the text that was matched by the term's
pattern. If you wish to simply return the string (e.g. for an Identifier term)
then some efficency can be gained by doing so as a move:

```
term <std::string> ID r:[a-z]+ <%{ return std::move(lexeme); }%>
```

#### Lexer Terminals

Lexer terminals are recognized by the the lexer but are not returned. They are
a means to skip over input that you do not want the grammar to consider.

Lexer terminals may not appear in rules.

Lexer terminals are defined by the `skip` keyword.

The patterns are specified the same way as for Parser Teminals.

```
// skip <ID> <"pattern"> ;
skip WS r:\s+ ;

// recognize line oriented comments
skip LINEC r://.*\n ;

// This is an ERROR!
rule Foo { => WS ; }
```


### Non-terminals
Rules are declared with the `rule` keyword.
Each alternate is introduced with `=>` and terminated with a semicolon.

One rule must be marked as the starting or "goal" rule, by preceeding it with the `goal` keyword.

```
rule MyRule {
  => MYTERM MyRule ;
  => ;  /* an empty alternative */
}

/* you can use single quoted patterns as aliases */
term SEMI ';' ;
term INT  'int';
rule A {
    => 'int' ID ';' ;
}

/* compact rule */
rule Compact { => A B ; => C Compact ; }

/* goal rule */
goal rule Program {
  => Program Statement ;
}
```

## Generated Code

Pre-pre-alpha. Subject to change.

*TODO:* Add info about the generated code. longest match rule, first match as
tie-breaker.

### Sample Driver

Here is all you need. Season to taste.

```cpp
#include "./YalrParser.hpp"

int main() {
    std::string input = "My Input";

    YalrParser::Lexer l(input.cbegin(), input.cend());
    auto parser = YalrParser::YalrParser(l);

    if (parser.doparse()) {
        std::cout << "Input matches grammar!\n";
        return 0;
    } else {
        std::cout << "Input does NOT match grammar\n";
        return 1;
    }
}
```

## References
- [Elkhound](http://scottmcpeak.com/elkhound/sources/elkhound/index.html)
- [Lemon](http://www.hwaci.com/sw/lemon/)
- [Boost::Spirit::X3](https://www.boost.org/doc/libs/develop/libs/spirit/doc/x3/html/index.html)
- [Grammophone](http://mdaines.github.io/grammophone/) - explore grammars.
- [LR on Wikipedia](https://en.wikipedia.org/wiki/LR_parser)
- [GLR on Wikipedia](https://en.wikipedia.org/wiki/GLR_parser)
- [Parser Notes](http://www.cs.ecu.edu/karl/5220/spr16/Notes/Bottom-up/index.html)
- [LALR(1) "Tutorial"](https://web.cs.dal.ca/~sjackson/lalr1.html)
- Recursive ascent-descent parsers
  - [Recursive ascent-descent
    Parsers](https://link.springer.com/content/pdf/10.1007/3-540-53669-8_70.pdf)
  - [Recursive ascent-descent
    parsing](https://webhome.cs.uvic.ca/~nigelh/Publications/rad.pdf)

## Technologies
- [Meson](https://mesonbuild.com/) for build configuration.
- [Ninja](https://ninja-build.org/) for building.
- [Catch2](https://github.com/catchorg/Catch2) for unit testing.
- [Boost::Spirit::X3](https://www.boost.org/doc/libs/develop/libs/spirit/doc/x3/html/index.html)
is currently used to build the grammar spec parser.
- [cxxopts](https://github.com/jarro2783/cxxopts) for command line handling.
- [inja](https://github.com/pantor/inja) to help with code generation.

## License

MIT &copy; 2018 Mark Hollomon
