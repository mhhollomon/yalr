# JSON Data structure for the code template

This document describes the layout of the JSON data used in the generating the code.

## Overview

The [inja](https://pantor.github.io/inja/) templating engine is used to render the final code. Inja uses JSON data - using the [nlohmann/json](https://github.com/nlohmann/json) library - to hold the values that will be inserted into the template.

The main template resides in src/include/template.hpp.

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

### Parser related data

- **states** : (array) The data for each state
  - **id** : (scalar) The numeric id of the state.
  - **actions** : (array) the actions for the state.
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
    - **itemtypes**  : (array) List of item types for this production.
    - **block**      : (scalar) actual code for the action.
    - **production** : (scalar) string describing the prod for this reduce
    - **rule_type**  : (scalar) type (eg. int, void) of the rule for this production.
