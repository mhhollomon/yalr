# Context Aware Lexing

## Motivating Example

Consider the following valid fragment of a `yalr` grammar :

```
// "r" is the marker that the following string is a regex and
// should be lexed as a single token along with the string.
term foo r:foo ;  
goal rule A { 
    // "r is an alias and needs to be lexed as an identifier.
    => r:foo <%{ compute_value(r); }%> 
}
```

The character sequence `r:foo` needs to be lexically analyzed in two different
ways depending on the context. In the context of the `term` statement it is a
single token of type REGEX. In the context of the `rule` statement it needs to
three tokens IDENTIFIER COLON IDENTIFIER.

## Classic Lexical Analysis

Classical Lexical Analysis is context free. The source stream is scanned and
divided into tokens based on low level analysis of that stream. While such
tools as [flex](https://github.com/westes/flex) can maintain state, the user
must be careful not to try to build a parser into the tool.

The "classical" parser communicates with the lexer through a very narrow interface -
asking it only "given me the next token"

[Bison](https://www.gnu.org/software/bison/) (and other such tools) generally
contain ways to control the lexer - basically allowing the user to more-or-less
easily build multiple lexers from a single specification. However,
co-ordinating the state of the lexeer with that of the parser (which in the
LR(1) case will be looking ahead) is error-prone.

For the motivating example, a flex/bison implementation would set up multiple
states in the flex created lexer and add actions to the bison production to
switch lexer states at the appropriate time. This allows the user to "hide" the
REGEX token possibility in those context wherre it is not valid.

In addition to being error prone, this can make error recovery quite difficult
since deciding on the correct state of the lexer can be difficult.

## Recursive-Descent and PEG parsing

RD and PEG parsers get around the problem by doing away with the separate
lexical analysis phase. The parser works directly on the stream of characters.
THis is reasonable since, at any one time, only a single grammar alternative is
being considered. Multiple possiblities are explored via back tracking.

However, pbuth RD and PEG parsers are even harder to resync after an error. And
they both suffer from an inability to handle left-recusive grammars. While in
many cases, the grammar can be re-written, the resulting parse tree will also
be different and not a natural fit to down stream processing.

## YALR and Context Sensitive Lexical Analysis

`Yalr` uses a distinct lexical analysis phase (like the flex/bison combo).
However, when asking the lexer for the next token, it passes to the lexer the
list of tokens that are valid for the current state of the parse. The lexer
only looks for those tokens in the character stream. The lexer will either
return on of the list, or it will return a special "error" token.

In essence, each state of the parser also acts as a state of the lexer -
allowing the lexer to work with full context information.

A naive gammar to recognize the motivating example just "works".

```
term IDENT r:_*[a-zA-Z]\w* ;
term REGEX r:r:[^\s]* ;

// ... other terms

goal rule program {
    => program rule_stmt ;
    => program term_stmt ;
    => rule_stmt ;
    => term_stmt ;
}

rule rule_stmt {
    => 'rule' IDENT '{' '=>' IDENT ':' IDENT ACTION '}'
}

rule term_stmt {
    => 'term' IDENT REGEX ';' ;
}
```

After seeing the IDENT in the term_stmt, the parser will move to a state where
REGEX is valid, but IDENT is not. The lexer will try to match REGEX and will
either do so successfully and return that token or it will return an error
token.

This is not without drawbacks.

### handling keywords

Consider the language that defines variables like :

```
var my_variable = 3
```

A first pass grammar for this would be :

```
term VAR 'var' ;
term IDENT r:[a-zA-Z]+ ;
term INTEGER r:[0-9]+ ;

goal rule declaration {
    => VAR IDENT '=' INTEGER ;
}
```

But consider the fragment :
```
var var = 3
```

This parses! After the parse sees the VAR token (the first "var"), it enters a
state where the only valid token is IDENT - but "var" looks like an IDENT.

The classic way to handle this is to have a list of keywords and have a
programmatic way for the lexer to reject or reclassify the match if the lexem
happens to be a keyword (See my [blog
post](https://www.codevamping.com/2018/09/identifier-parsing-in-boost-spirit-x3/)
for solving this problem when using [Boost::Spirit
X3](https://www.boost.org/doc/libs/develop/libs/spirit/doc/x3/html/index.html)
)

To solve for this in `yalr`, a terminal can be marked as "global". Such a
terminal is always considered regardless of what permissible tokens are
requested by the parser.

```
global term VAR 'var' ;
term IDENT r:[a-zA-Z]+ ;
term INTEGER r:[0-9]+ ;

goal rule declaration {
    => VAR IDENT '=' INTEGER ;
}
```

This can be a two-edged sword since there is currently no way to "un-global" a
terminal. It would not be possible to have two separate groups of keywords that
are reserved in different contexts.

!! {TODO} - Add the cool stuff that allow error recovery - e.g. token
substitution, input scanning, etc.
