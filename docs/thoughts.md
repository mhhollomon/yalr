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
would work if the type was `void`. 
~~A void symbol doesn't have a value, so you can't reference it~~

In the second alternative, the `std::optional` would be set to `std::nullopt`.

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

This would also set up the machinery to do things like `+()` and `*()` and
maybe true "inline rules" using `|`.

This version ALSO has issues if the symbol is of type `void`.
~~No, it wouldn't, a void symbol doesn't have value, so you can't reference
it~~

SLR(1) may have trouble with reduce/reduce conflicts if there are a bunch of
these in the grammar. consider:

```
rule A { => maybe_B C D ; => maybe_E F G ; }
```

If the intersection of FIRST(C) and FIRST(F) is not empty, the grammar would
have a reduce/reduce on those terminals.

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

**bleh** No named parameters to functions in c++20

In other words, it would automatically call the constructor with named
parameters. Only those items with alises would participate.

Maybe one step further and :

```
rule <@auto> bar { => x:A c d y:e ; }
```

would create an actual struct definition as well :
```
preamble parser <%{ struct bar { typeA x; type_e y; }; }%>

rule <bar> bar { => x:A c d y:e <%{ return bar{ _v1, _v4 }; %}> }
```

Should these return `std::unique_ptr` or `std::shared_ptr` instead?

Have a way for the user to decide?

## @tree

Another thought around the `@auto` thing. Maybe instead have:

```
rule <some_class>Foo { => a:this b:that c:OTHER @tree(a,c,b) }
```

Transform into something like:

```
rule <some_class>Foo {  
    => a:this b:that c:OTHER 
            <%{ return std::make_unique<some_class>(a, c, b); }%>
}
```

and for the entire rule :

```
rule <some_class>Foo @tree(a,b,c) {
    => a:this b:that c:OTHER ;
    => b:that c:SOMETHING ;
    }
```

transforms to:

```
term <int> this 'this' ;

rule <some_class> Foo {
    => a:this b:that c:OTHER <%{ return std::make_unique<some_class>(a, b, c); }%>
    => b:that c:SOMETHING <%{ return std::make_unique<some_class>(int{}, b, c); }%>
```

In other words, missing parameters get default initialized objects of the
correct type.

`@auto` might still be a better name.
