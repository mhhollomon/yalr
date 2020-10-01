 list of random thoughts for improvements or changes - in no particular order -
with no idea if or when they get implemented.

### Type of untyped terminals
Currently, if a terminal (or rule) is not explicitly given a type, then the
type is `void`. It will have no semantic value. And indeed, its semantic
variable will not even be defined.

```c++
term <int> INT r\d+ <%{ return atoi(lexeme); }%>

// This won't compile becaouse _v2 won't be defined.
rule <int> foo => { INT '+' INT <%{ std::cout << _v2 << "\n"; return _v1 + _v3 }%> }
```

Instead, would it be better to make it return the Token value?

Pro: would make writing some forms of ast generation easier since the following
could be done.

```C++
rule <int> foo { 
    => INT '+' INT <%{ return compute_func(_v2, _v1, _v3) }%>
    => INT '-' INT <%{ return compute_func(_v2, _v1, _v3) }%>
}
```

Cons:
1. would have to expose the token enum to the users somehow.
   - note that they can get to it now anyway as YalrParser::token_type
2. what would we do about rules? Same thing?


### optional item

```C++
rule rule_def { => ?'goal' 'rule' ...  <%{ /* action */ }%> }
```

Two possible ways to accomplish this.

#### Extra alterative
The above could expand to:

```C++
rule rule_def {
     => 'goal' 'rule' ... <%{ /* action */ }%>  
     => 'rule' ...   <%{ /* action */ }%>
}
```

The value from `'rule'` would be mapped to `_v2` in both cases.  `_v1` would be
a `std::optional` of whatever type the `'goal'` returns. Not sure how that
would work if the type was `void`. In the second alternative, the
`std::optional` would be set to `std::nullopt`.

#### Extra rule
The example would expand to:

```C++
rule rule_def { => maybe_goal 'rule' ...  <%{ /* action */ }%> }
rule <std::optional<"type of 'goal'">> maybe_goal {
    => 'goal' <%{ return _v1; }%>
    => <%{ return std::nullopt; }%>
```

This may be easier to pull off as it doesn't ply dirty tricks with the semantic
value numbering. And since the users action block is not duplicated, it may
make the compilation messages a bit easier to under stand.

This would aslo set up the machinery to do things like `+()` and `*()` and
maybe ture "inline rules" using `|`.

This version ALSO has issues if the symbol is of type `void`.

### Simplify the spec.
Get rid of the braces around rules.
Get rid of the `rule` keyword.

```
foo <int> => left:INT '+' right:INT  <%{ return left + right }%>
```

### <@auto:foo> for rules

How about allowing this:

```
rule <@auto:foo> bar { => x:A c d y:e ; }
```

which would get rewritten as :

```
rule <foo> bar { => x:A c d y:e <%{ return foo(x=_v1, y=_v4); }%>
```

In other words, it would automatically call the constructor with named
parameters. Only those items with alises would participate.

Maybe one step further and ;

```
rule <@auto> bar { => x:A c d y:e ; }
```

would create an actual struct definition as well :
```
preamble parser <%{ struct bar { typeA x; type_e y; }; }%>

rule <bar> bar { => x:A c d y:e <%{ return bar{ _v1, v_2 }; %}> }
```

Should these return `std::unique_ptr` or `std::shared_ptr` instead?

Have a way for the user to decide?


### termset

```
termset <int> MYSTUFF @prec=200 'a' 'b' 'c' <%{ return int(lexeme[0]); }%>
```

should expand to:

```
term <int> __1 'a' @prec=200 <%{ return int(lexeme[0]); }%>
term <int> __2 'b' @prec=200 <%{ return int(lexeme[0]); }%>
term <int> __3 'c' @prec=200 <%{ return int(lexeme[0]); }%>

rule <int> MYSTUFF {
    => 'a' <%{ return _v1; }%>
    => 'b' <%{ return _v1; }%>
    => 'c' <%{ return _v1; }%>
}
```

It is fine to use an already defined terminal. However, if the termset set
precedence, then the used terminal cannot already have precedence set, or it
will result in an analysis phase error.

Also the type of the terminal must match the termset - unless the termset shows
void.

```
// termset is void
term <int> A 'a' <%{ return 42; }%>
termset FOO A 'b' 'c';

// ERROR - term is void ut termset is int
term A 'a' ;
termset <int> A 'b' <%{ ... }%>

// Okay - no precedence
term A 'a' ;
termset FOO @prec=200 A 'b' 'c' ;

//Okay - no termset precedence
term A 'a' @prec=200 ;
termset FOO A 'b' ;

// ERROR - precedence conflict - even though
// they are the same number
term A 'a' @prec=200 ;
termset FOO @prec=200 A 'b' ;
```
