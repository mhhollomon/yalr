# Option Handling

[Class Diagram](svg/options.svg)

## Overview

Options are stored in an `option_table` object. Each known option has a public
member of a type derived from the CRTP base class `option_base`.

There are `option_table` objects in both the `analyzer_tree` and `lrtable`
structures.

## using option_table

There are two different "names" that an option is known by. The first name is
the actual name of the member varialbe in the `option_table` class. The second
name is a string that is the name as it would be called in yalr grammar file.

The two names are normally related to each other with the member name being the
grammar name with dots replaced with underbars. But a few a very different
because the grammar has specific statement types:

option name | member name
-------------------------
code.main   | code_main
lexer.case  | lexer_case
lexer.class (lexer class statement) | lexer_class
parser.class (parser class statement) | parser_class
code.namespace (namespace statement)   | code_namespace

To get the value of a particular option use `get()` on the option member.

To set the value of the  option, use `validate()` on the `option_table` with
the option name.

```cpp
option_table ot;

// set the namespace
if (not ot.validate("code.namespace", "one::two")) {
    /* error */
}

// get the namespace
auto ns = ot.code_namespace.get();

// check if an option name is valid
if (ot.contains("foo.baz")) {
    /* all good */
}
```

## Two phase create

In order to ease maintenance, an architecture was created where the option
members register themselves - along with validation function and name - to the
parent. To ensure that there would be no probems with initialization order, the
`option_table` class was split into two pieces. THe base class
`_option_table_base` has the map between option names and validation funtions
and the handles registering the individual members.

The "external" class `option_table` then handles the external api.

This works because base classes are guaranteed to be completely contructed
before any construction or initialization of derived class members.

The complete initialization for an `option_table` objects is as follows:

1. The `_option_table_base` subobject is initialized. This makes sure the
   `unordered_map` is ready.

2. Each of the members of the `option_table` are initialized in turn.

    1. the option\<T,Derived> subobject is initialized. The constructor
       registers the new option with the parent - which winds up calling into
       the `_option_table_base` subobject and gets added to the map.

## Adding a new option

You will need to know three things.

1. Storage - what kind of value will the option be (bool, int, enum)?

2. Can it change? - Can the value of the option vary during parsing the
   grammar, or should it have a single value for the entire time?

3. How to validate - The parser will give you a string_view. You will need a
   way to turn that into a proper value and detect if that cannot be done.

Create a new class that derives from the `option` class. The answer to question
1 will be used as a template parameter to the base class.

The class' constructor must take 3 arguments in the following order:

argument | comment
---------|--------
std::string_view v | The "name" of the option
\_option_table_base& p | The parent option_table. This is not stored, but simply used to call the `_register()` method.
T def    | Default value.

The answer to question 2 will be used to initialize the `option` base
subobject.

The class must define a member function `validate(std::string_view)` that
returns a `bool`. This method is required to validate the value as represented
by the `string_view` and, if it passes, set the option to the required value.

see `struct lexer_case_option` for an example.
