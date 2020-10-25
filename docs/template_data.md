# JSON Data structure for the code templates

This document describes the layout of the JSON data used in the generating the code.

## Overview

The [inja](https://pantor.github.io/inja/) templating engine is used to render
the final code. Inja uses JSON data - using the
[nlohmann/json](https://github.com/nlohmann/json) library - to hold the values
that will be inserted into the template.

The code is broken up into a several different templates. All reside in the
`src/include/templates` directory.

- preamble  : stuff before the lexer
- lexer     : The lexer itself
- parser    : each algorithm (slr, clr, lalr) has its own template.
- postlude  : Anything that is needed after the parser.
- main_func : The definition of `main()`. Only applied if the user requested it.

## Data Layout

### Global

- **namespace** : (scalar) The namespace to wrap the lexer and parser in.
- **parserclass** : (scalar) The name to give the parser class
- **lexerclass**  : (scalar) The name to give the lexer class

### Lexer related data

- **enums** : (array) the list of tokens
    - **name**  : (scalar) the name of the enum entity
    - **value** : (scalar) the value to give the entity
- **types** : (array) list of type names. Used to create the values variant.
- **semantic_actions** : (array) data related to terminal's actions.
    - **token** : (scalar) Token that owns the action.
    - **block** : (scalar) Actual code for the action.
    - **type**  : (scalar) Type of the expected returned value.
- **patterns** : (array) Data surround the matching logic for terms and skips.
    - **matcher** : (scalar) The type of matcher - string or regex.
    - **pattern** : (scalar) The actual thing to match.
    - **token**   : (scalar) The token that owns the match.
    - **is_global** : (scalar) True iff the terminal was marked as 'global'.
- **pattern_count** : (scalar) Number of patterns.

### Parser related data

- **states** : (array) The data for each state
  - **id** : (scalar) The numeric id of the state.
  - **actions** : (array) the actions for the state.
      - **token**        : (scalar) input token that triggers the action
      - **type**         : (scalar) type of action, reduce, shift, accept
      - **prodid**       : (r, scalar) Numeric id of the production.
      - **production**   : (r, scalar) string describing the prod for a reduce
      - **count**        : (r, scalar) Number of items in the production
      - **returnlevels** : (r, scalar) Number of levels to skip in the return
      - **symbol**       : (r, scalar) Name of Token for the rule
      - **valuetype**    : (r, scalar) Type string (e.g. 'int', etc).
      - **hasvaluetype** : (r, scalar) Boolean - is the valuetype not 'void'
      - **hassemaction** : (r, scalar) Boolean - prod has a semantic action.
      - **newstateid**   : (s, scalar) For shift, the number of next state to enter
  - **gotos** : (array) The gotos for the state
      - **symbol**  : (scalar) The token name of the look ahead symbol
      - **stateid** : (scalar) The numeric id of the new state.
- **reducefuncs** : (array) The data for each reduce function
    - **prodid**     : (scalar) Numeric id of the production.
    - **hassemaction** : (scalar) Boolean - prod has a semantic action.
    - **itemtypes**  : (array) List of item types for this production.
        - **type**  : (scalar) string for the type of the semantic value for
            this item
        - **alias** : (scalar) string for the alias for this item.
        - **index** : (scalar) number of this item in production.
    - **block**      : (scalar) actual code for the action.
    - **production** : (scalar) string describing the prod for this reduce
    - **rule_type**  : (scalar) type (eg. int, void) of the rule for this production.
