# Flint Docs
---
### 1. Data Types
- `num` = number *(including both ints/floats)*
- `text` = string *(syntax is `"string"/'string'`)*
- `bool` = boolean *(`true`/`false`)*
- `list` = ordered, changable array *(syntax is `[item, item, item]`)*
- `map` = like python `dict` *(syntax is {key: value, key: value})*
- `null` = like python `None`
---
**To be added?:**
- `file` = file handle *(`let f = file "notes.txt"`, etc.)*
- `sprite` = graphical entity, useful for game dev *(`let player = sprite "cat.png"`, etc.)*

### 2. Comments
- single-line: `-- comment`
- multi-line: `#- comment -#`

### 3. `start`
```
start:
    DO_SOMETHING
    ...
```
Code in `start:` will run on executing the program. Any code that isn't in `start:` or isn't referenced in `start:` will not run. 

### 4. `write` Command
`write EXPRESSION`

Example: `write "Hello World"`
- writes `EXPRESSION` into console
- `write` is similar to `print()` in python or `console.log()` in js
- brackets are not needed (same with the other functions)

### 5. Variable Assignment (`=`)
`VARIABLENAME[:TYPE] = EXPRESSION`

Example: `x = 10`, `y: list = ["a", "b"]`
- declares a variable `VARIABLENAME` with value `EXPRESSION`
- you can optionally indicate the type of the variable
- `VARIABLENAME` must only contain alphanumeric characters and underscores (A-z, 0-9, \_), and cannot start with a number

### 6. Operators
`EXPRESSION OPERATOR EXPRESSION`

The basic arithmetic, comparison, and logical operators are the same as Python. Parentheses can also be used to group expressions similar to Python.

**6.1. Arithmetic:**
- `+`: adds both `num`s
- `-`: subtract left `num` from right `num`
- `*`: multiply both `num`s
- `/`: divide left `num` from right `num`
- `%`: (left `num`) modulo (right `num`)

**6.2. Comparison:**
All expressions with comparison operators output a `bool` value (`true`/`false`)
- `==`: return `true` if both expressions are equal
- `!=`: return `true` if both expressions are different
- `<`/`>`: left less/greater than right
- `<=`/`>=`: left less/greater than or equal to right

**6.3. Logical**
Logical operators are used to combine conditional statements:
- `and`: return `true` if both `bool`s are `true`
- `or`: return `true` if at least one `bool` is `true`
- `not`: invert a `bool`
`!` can also be used to invert a boolean (`!true` = `false`, `!(2 == 3)` = `true`)

### 7. Control
The `:` character, along with indents, define block structures.

**7.1. `if` Statements**
`if` statements are the similar to Python `if` statements:
```
if BOOL:
    DO_SOMETHING
else:
    DO_SOMETHING_ELSE
```
`elif` statements does not exist in Flint, however. They can be substituted with nesting if-else statements, or `check` statements (mentioned later)

**7.2. `while` Loop**
```
while BOOL:
    DO_SOMETHING
```
`while` repeatedly executes the code inside the loop as long as `BOOL` remains `true`.

The keywords `break` and `continue` can be used within `while` loops. The `break` statement exits the innermost enclosing loop, and the `continue` statement skips to the next iteration of the loop.

**7.3. `loop` Command**
```
loop N:
    DO_SOMETHING
```
The `loop` command provides a simple way to repeat code a specific number of times. Replace `N` with any expression that evaluates to a number. The code block will be executed exactly `N` times.

**7.4. Defining Commands**
```
command COMMAND_NAME [ARGUMENTS ... ] :
    DO_SOMETHING
```
`command` defines a new temporary command in the scope it is run in. Temporary commands are also called functions in other languages. They can be called the same way system commands are called (`new-command [ARGUMENT ... ]`)

**7.5. `check` Statements**
```
check EXPRESSION:
    equals EXPRESSION1:
        DO_SOMETHING
    equals EXPRESSION2:
        DO_SOMETHING_ELSE
    ...
```
A check/equals statement is an alternative to nested `if` statements. It takes an expression and compares its value to successive patterns given as one or more case blocks. This is similar to switch statements in other languages.

### 8. Text Manipulation
**8.1. Interpolation**
`"...${EXPRESSION}..."`

Example: `name = "World"`, `write "Hello, ${name}!"` outputs `Hello, World!`
`${}` embeds expressions inside `text`. Use `\$` to use a literal `$`.

**8.2. `lower`/`upper`**
`lower | upper text:TEXT`

Converts `TEXT` into lowercase/uppercase.

**8.3. `trim`**
`trim text:TEXT`

Remove whitespaces from both sides of `TEXT`.

**8.4. `reverse`**
`reverse text:TEXT`

Reverse `TEXT`.

### 9. Ask Statement
`ask TEXT [as VARIABLE]`

`ask` is similar to `input()` in Python. It writes `TEXT` to standard output without a newline and returns the input converted to a `text`. If `as VARIABLE` is given, it declares `VARIABLE` and assigns the input to it.

### 10. Type Conversion
`num | text | bool EXPRESSION`

`num | text | bool` will convert `EXPRESSION` to their respective types.
- `num` can convert `text` containing only numerals and convert `true`/`false` to `1`/`0`
- `text` can convert any `num` to its corresponding characters and `bool` to `"true"`/`"false"`
- `bool` can only convert `text` with `"true"`/`"1"`/`"false"`/`"0"` or `num` with `1`/`0`

### 11. Objects
```
object NAME:
    ATTR_NAME = VALUE
    ...
```
Example:
```
object Player:
    name = ""
    score = 0

let p = Player()
p.name = "Alice"
```
`object` creates a new object that can be added with attributes with `ATTR_NAME = VALUE`. This allows new instances of the object to be made. The attribute(s) of `OBJECT` can be set with `OBJECT.ATTR_NAME = VALUE`. This is heavily simplified from other OOP languages' objects.

### 12. `wait`
`[async] wait SECONDS`

Running `wait` will pause the entire program by `SECONDS` seconds, similar to Python's `time.sleep()` function.

### 13. Random
`random` is a built-in module that implements pseudo-random number generators.
- `random.seed num:SEED`: sets the seed for psuedo-random number generation, if not specified, the system time will be used
- `random.int num:START num:STOP`: returns a random integer `num` in the range `START < X <= STOP`, `START` and `STOP` will be set to 0 and 100 if not set
- `random.float num:START num:STOP`: returns a random float `num` in the range `START < X <= STOP`, `START` and `STOP` will be set to 0 and 100 if not set

### 14. Piping
`COMMAND1 |> COMMAND2 |> COMMAND3 | ... `

Example:
`"EURT" |> lower |> reverse |> bool |> write -- output = true`

The `|>` operator passes the output of the command or expression to the left to the input of the command to the right. This can be chained to apply commands in order.




