# Yet Another LR Parser Generator
## Yalr Release 0.1.2
[![Github Releases](https://img.shields.io/github/release/mhhollomon/yalr.svg)](https://github.com/mhhollomon/yalr/releases)
[![Build Status](https://api.cirrus-ci.com/github/mhhollomon/yalr.svg)](https://cirrus-ci.com/github/mhhollomon/yalr)
[![Github Issues](https://img.shields.io/github/issues/mhhollomon/yalr.svg)](http://github.com/mhhollomon/yalr)
[![GitHub License](https://img.shields.io/badge/license-MIT-blue.svg)](https://raw.githubusercontent.com/mhhollomon/yalr/master/LICENSE)

## Status

Yalr is fully functional - if bare-boned. See the [Calculator
example](examples/calculator.yalr)

## Overview

Yalr is yet another LR (actually SLR) compiler generator.

The design goal was to create an generator that created a single file, making
it easy to integrate into a build system. The code generated is C++17.

## Building

Yalr requires CMake, make, and a C++17 compliant compiler. Assuming you have those, then building is:

```bash
git clone https://github.com/mhhollomon/yalr.git
cd yalr
./scripts/build.sh release
```

The script will build and run the tests.

To install :

```bash
sudo cmake --install build-release
```

## Running

At the simplest, it is:

```
yalr my_grammar.yalr
```

This will generate a file `my_grammar.yalr.hpp` that has the generated parser in it.

Other options:

```
# get help
yalr -h
yalr --help

# Rename the output file
yalr -o foo.hpp my_grammar.yalr

# Instead of outputting the parser,
# translate the grammar for use on grammophone
# (see references)
yalr -t grammophone my_grammar.yalr
```

## Grammar Spec

Whitespace is generally not significant. `C` style `/* ... */` comments 
as well as C++ `//` comments are supported.

Keywords are reserved and may not be used as the name of a terminal or rule.

The [example
directory](https://github.com/mhhollomon/yalr/tree/master/examples) contains
some example grammars including a (probably out-of-date) grammar for the yalr
grammar itself.

### Parser Class Name

The parser is normally put in `class Parser`. This can be changed by using the statement:
```
parser class MyClass;
```
This statement may only appear once in the file.

### Lexer Class Name

The lexer is normally put in `class Lexer`. This can be changed by using the statement:
```
lexer class MyClass;
```
This statement may only appear once in the file.

### Namespace

Both the lexer and and parser are normally put into the namespace 'YalrParser'.
This can be changed by using the statement:
```
namespace MySpace;

// or use single quotes for inline namespaces:
namespace 'one::two';
```

This statement may only appear once in the file.

### Terminals

There are two types of terminals - "parser" terminals and "lexer" terminals.

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
The pattern can be used for the term in rules.

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

The action should be thought of as the body of a lambda that returns the
semantic value to be passed back to the parser. The identifier `lexeme` is
available. It is a string that contains the text that was matched by the term's
pattern. If you wish to simply return the string (e.g. for an Identifier term)
then some efficency can be gained by doing so as a move:

```
term <std::string> ID r:[a-z]+ <%{ return std::move(lexeme); }%>
```

##### Terminal Precedence and Associativity

Parser terminals can be assigned a precedence and associativity in order to
help resolve possible conflicts in the grammar. Please see the section below
on [Action Conflict Resolution](#action-conflict-resolution) for details on the
algorithm.

Associativity is assigned to the terminal using the `@assoc=` flag. It can be
assigned either `left` or `right` associativity. By default, terminals are
assigned an `undef` associativity.

```
term Mult '*' @assoc=left ;
```

The `left` or `right` keyword must come directly after the flag. There can be
no spaces btween the equal sign and the value.

Precedence is assgined to the terminal using the `@prec=` flag. It can be
assigned as a positive integer value, or as the name or pattern of another
terminal. The referenced terminal must have a precedence assigned.

```
term Mult '*' @assoc=left @prec=200 ;

// give Div the same precedence as Mult
term Div  '/' @assoc=left @prec='*' ;
```

Higher precedence values "bind tighter".

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


### Associativity statement

Terms can be given an assoviativity setting using the `associativity`
statement. This statment will also create single-quote style terminals "inline"

```yalr
term foo 'foo';
associativity left foo 'x' 'y' ;
```

If a pre-existing terminal is mentioned in an a `associativity` statement, it
cannot already have its associaitivty set.

```yalr
term foo 'foo' @assoc=left;

// !ERROR!
associativity left foo 'x' 'y';
```
### Precedence statement

Terminals can be given a precedence setting by using the `precedence`
statement. This statement will also create single-quote style terminals
"inline".

The precedence can be given either as a number, or as a reference to an
existing terminal that has precedence set.

```yalr
term foo 'foo';
precedence 200 foo 'x' 'y';
```

If a pre-existing terminal is mentioned in a `precedence` statement, it cannot
already have it precedence set.

```yalr
term foo 'foo' @prec=200 ;
// !ERROR!
precedence 200 foo;
```

### Rules

Rules are declared with the `rule` keyword.
Each alternative is introduced with `=>` and terminated with a semicolon.

One rule must be marked as the starting or "goal" rule, by preceeding it with
the `goal` keyword.

An alias may be given to each symbol in the alternate. The value of that symbol
will then be available in the action block.

A terminal whose pattern is a single-quoted string may be referenced either by
the name given it, or by the pattern itself (complete with the quotes).

If a single-quoted string is used in a rule, but no terminal has been defined
with that string, then one is automatically created. While this can be very
convenient, it does not allow you to assign a type or an action/value to the
terminal. But for common structural lexemes (like semi-colon and the like),
this may actually be quite helpful. This can also make the rules a bit easier
to read since they will read more like the string they would match.

#### Production sematic action

Each production in a rule can be assigned its own action to compute a semantic
value. The semantic values of the items in the production are available to the
actions in variables of the form `_v{n}` where `{n}` is the position of the
item from the left numbered from 1. An item may also be given an alias. This
alias will be used to create a reference variable that points to the
corresponding semantic avalue variable. If an represents a rule or terminal
without a type, the corresponding semantic variable will not be defined.
Giving an alias to such an item will result in an error. 

```
rule MyRule {
  => MYTERM MyRule ;
  => ;  /* an empty alternative */
}

/* you can use single quoted patterns directly in the rule */
/* The system will define a terminal for ';' for us */
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

// semantic actions
term <int> NUM r:[-+]\d+ <%{ return std::stoi(lexeme); }%>
term ADD 'add' ;

// Using the underlying semantic variables
// Note that the 'sub' takes up _v1 - even though it does not have a type.
rule <int> RPN_SUB { => 'sub' NUM NUM <%{ return _v2 - _v3; }%> }
// With aliases
rule <int> RPN_ADD { => 'add' left:NUM right:NUM <%{ return left + right; }%> }
```

#### Rule Precedence

An alternative may be assigned an explicit precedence using the same `@prec=`
flag as terminals. The flag must go after the last item but before the closing
semi-colon or action block.

If an alternative is not given an explicit precedence, then the alternative
will have the same precedence as the *right-most* terminal in the alternative.
If it has no terminals, then it will have the same default precedence as a
terminal that has no defined precedence.

```
term X 'x' @prec=200 ;

rule S {
    => 'a' 'x' E           ; // this will have a prec=200
    => 'a'  E  E           ; // default precedence
    => 'x' 'a' E           ; // also default
    => 'x' 'a' R @prec='x' ; // this will have prec=200
}
```

## Verbatim Code Injection

Sections of code may be injected into the generated code via the `verbatim`
statement.

```
verbatim $location $actionblock
```

The possible locations are:

location | description
---------|-------------
file.top | Just after the generated includes
file.bottom | Just before the closing inclusion guard
namespace.top | Just after openng the namespace
namespace.bottom | Just before the closing brace for the namespace
lexer.top | Near the top of the lexer class definition
lexer.bottom | just before the closing brace for the lexer class
parser.top | Near the top of the parser class, after helper routines, but before the state methods.
parser.bottom | Just before the closing brace for the parser class

```
verbatim file.top <%{
#include "project.hpp"
}%>
```

## Action Conflict Resolution

Sometimes in a grammar, the parser will have states where there are two
possible actions for the given state and input.

Consider this grammar fragment:
```
rule E {
    => E '+' E ; // production 1
    => E '*' E ; // production 2
    => number  ; // production 3
}
```

and the input `1 + 2 * 3`.

Would like it to parse it as `1 + ( 2 * 3)` - that is - use the second
production first and then use the first production to create the parse tree :
```
E(+ E(1) E(* E(2) E(3)))
```

The important point is after it has seen (and shifted) '1' '+' '2' and is
deciding what to do with the `*`. it has a choice, it can shift it and delay
reducing until later (this is what we want it to do), or it can go ahead and
reduce by production 1.  

When there is a shift/reduce conflict like this thegenerator will compare the
precedence of the production (1) and the terminal(`*`). If the production is
greater, then the reduce will be done. If the terminal is higher precedence,
then the shift will done. 

If the two have equal precedence, the associativity of the terminal will be
consulted. If it is 'left' then reduce will be done. If it is 'right', then the
shift will be done.

If `yalr` cannot decide what to do, an error will be generated.

It is also possible to have two rules come in conflict (reduce/reduce). The
same rules apply.

So, to make our example act as we want, we need to make `*` have a higher
precedence than production 1. By default it will have the precedence of the `+`
terminal.

```
term P '+' @prec=1
term M '*' @prec=200

rule E {
    => E '+' E ; // production 1
    => E '*' E ; // production 2
    => number  ; // production 3
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

See [calculator_main](examples/calculator_main.cpp) for a larger example.

## References
- [Elkhound](http://scottmcpeak.com/elkhound/sources/elkhound/index.html)
- [Lemon](http://www.hwaci.com/sw/lemon/)
- [Boost::Spirit::X3](https://www.boost.org/doc/libs/develop/libs/spirit/doc/x3/html/index.html)
- [ANTLR](https://www.antlr.org/)
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
- [Cmake](https://cmake.org/) for build configuration.
- [doctest](https://github.com/onqtam/doctest) for unit testing.
- [cxxopts](https://github.com/jarro2783/cxxopts) for command line handling.
- [inja](https://github.com/pantor/inja) to help with code generation.

## License

MIT &copy; 2018 Mark Hollomon
