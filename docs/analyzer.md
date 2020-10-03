```
term 'a' ;
term B 'b' ;
term C r:ce+ ;

assoc left 'a' 'b' ; // Can't create
prec  500 'a' 'b' C ; // Can't create

termset FOO C 'b' 'x' 'y' 'z' ;

termset <@lexeme> BAR 'u' 'w' ;

goal rule RULE1 {
    =>  RULE1 FOO ;
    =>  BAR ;
    =>  Zed 'x' ; // NOPE - Zed not defined
    }

prec 200 Zed ; // NOPE - Zed not yet defined
term Zed 'z' ; //OKAY

term 'x' ; // NOPE - already defined in the rule
```

## Pass 1

Pass 1 is dedicated to building out the symbol table for rules names. Since we
must allow mutually recursive rules, it must be possible to use a rule before
we actually see its definition.

For terminals, they must be defined before they are used - or at least for
single quote fixed string terms, defined at the place of first use.

The other statement types play no part in this pass.

### rule
- Register the name of the rule as a symbol.

### termset
- Register the name of the termset as a rule symbol.

## Pass 2

Now that rule name look-ahead is done, we can do most of the other processing
in line.

Pass 2 builds out the rest of the symbol table and updates the table to reflect
changes in associativity, precedence, match type, and other options.

### option (and friends)
- update the option table

### term
- Register name (and maybe pattern) as terminal symbol
    - check if unique and warn
- Do `@lexeme` processing

### skip
- Register name (and maybe pattern) as terminal symbol
    - check if unique and warn

### assoc
- Update terms as needed with appropriate guards

### prec
- Update terms as needed with appropriate guards

### rule
- For each alternative
    - If the rule type is non-void check that there is an action.
    - for each item in the production
        - check that item is in the symbol table
        - if it is absent AND is a SQ pattern, register it
    - register the production in analyzer output tree.

### termset
- Check that the type is either void or @lexeme
- For each item
    - If it exists in the symbol table, check that the type is compatible
    - IF it doesn't exist AND is an SQ pattern, register it
    - register a one item production
