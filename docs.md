# Flint Docs
---
### 1. Data Types
- `num` = number *(including both ints/floats)*
- `text` = string *(syntax is `"string"`)*
- `bool` = boolean *(`true`/`false`)*
- `list` = ordered, changable array *(syntax is `[item, item, item]`)*
- `map` = like python `dict` *(syntax is {key: value, key: value})*
- `null` = like python `None`
---
**To be added?:**
- `object` = user-defined OOP-style object *(`object Player: ...`)*
- `file` = file handle *(`let f = file "notes.txt"`, etc.)*
- `sprite` = graphical entity, useful for game dev *(`let player = sprite "cat.png"`, etc.)*

### 2. Comments
- single-line: `-- comment`
- multi-line: `#- comment -#`

### 3. `write` Command
`write EXPRESSION`

Example: `write "Hello World"`
- writes `EXPRESSION` into console
- `write` is similar to `print()` in python or `console.log()` in js
- brackets are not needed (same with the other functions)

### 4. Variable Assignment (`=`)
`VARIABLENAME[:TYPE] = EXPRESSION`

Example: `x = 10`, `y: list = ["a", "b"]`
- declares a variable `VARIABLENAME` with value `EXPRESSION`
- you can optionally indicate the type of the variable

### 5. Operators
`EXPRESSION OPERATOR EXPRESSION`

The basic arithmetic, comparison, and logical operators are the same as Python. Parentheses can also be used to group expressions similar to Python.

**5.1. Arithmetic:**
- `+`: adds both `num`s
- `-`: subtract left `num` from right `num`
- `*`: multiply both `num`s
- `/`: divide left `num` from right `num`
- `%`: (left `num`) modulo (right `num`)

**5.2. Comparison:**
All expressions with comparison operators output a `bool` value (`true`/`false`)
- `==`: return `true` if both expressions are equal
- `!=`: return `true` if both expressions are different
- `<`/`>`: left less/greater than right
- `<=`/`>=`: left less/greater than or equal to right

**5.3. Logical**
Logical operators are used to combine conditional statements:
- `and`: return `true` if both `bool`s are `true`
- `or`: return `true` if at least one `bool` is `true`
- `not`: invert a `bool`
`!` can also be used to invert a boolean (`!true` = `false`, `!(2 == 3)` = `true`)

### 6. Control
The `:` character, along with indents, define block structures.

**6.1. `if` Statements**
`if` statements are the similar to Python `if` statements:
```
if BOOL:
    DO_SOMETHING
else:
    DO_SOMETHING_ELSE
```
`elif` statements does not exist in Flint, however. They can be substituted with nesting if-else statements, or `compare` statements (mentioned later)

**6.2. `until` Loop**
```
until BOOL:
    DO_SOMETHING
```
`until` repeatedly loops the code inside the loop *until* `BOOL` is `true`. Python `while` loops can be implemented with `until` by simply inverting the `BOOL`.

The keywords `break` and `continue` are exclusive to the `until` loop. The `break` statement *breaks* out of the innermost enclosing `until` loop, and the `continue` statement *continues* with the next iteration of the loop.

**6.3. Defining Commands**
```
command COMMAND_NAME [ARGUMENTS ... ] :
    DO_SOMETHING
```
`command` defines a new temporary command in the scope it is run in. Temporary commands are similar to functions in other languages. They can be called the same way system commands are called (`new-command [ARGUMENT ... ]`)

**6.4. `compare` Statements**
```
compare EXPRESSION:
    equals EXPRESSION1:
        DO_SOMETHING
    equals EXPRESSION2:
        DO_SOMETHING_ELSE
    ...
```
A compare/equals statement is an alternative to nested `if` statements. It takes an expression and compares its value to successive patterns given as one or more case blocks. This is similar to switch statements in other languages.
