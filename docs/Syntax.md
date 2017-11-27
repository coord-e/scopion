# Syntax
**This document is written in progress.**
See [Parser.cpp](lib/parser/parser.cpp) for full syntax definition.

## Identifier
```EBNF
identifier ::= alpha (alnum | '_' )*
```

Identifier is used to identify variables, structure's keys, etc...

## Primary Value
```EBNF
primary ::= int | bool | string | raw_string | variable |
    pre_variable | structure | array |
    function | scope | ('(' expression ')');
```

### Literals
#### Integer
An integer literal expresses signed 32-bit integer literal.
#### Bool
A bool literal expresses true or false.
#### String
```EBNF
string ::= ('"' ("\"" | char - '"')* '"');
raw_string ::= (''' ("\'" | char - '\'')* ''');
```

A string (with "") has escape sequences. To use " in raw string, use \".

| Sequence | Character       | Code |
|:---------|:----------------|:-----|
| \n       | NEWLINE         | 0x0a |
| \r       | CARRIAGE RETURN | 0x0d |
| \t       | HORIZONTAL TAB  | 0x09 |
| \v       | VERTICAL TAB    | 0x0b |
| \b       | BACKSPACE       | 0x08 |
| \f       | NEWPAGE         | 0x0c |
| \a       | AUDIBLE BELL    | 0x07 |
| \\       | \               | 0x5c |

A raw string (with '') doesn't have escape sequences.  To use ' in raw string, use \'.
Both string literal style can include a newline. For example, both of them below is legal code.
```
str = "Hello, World.
Hogehoge";
rstr = 'Raw string.
Piyopiyo';
```

#### Function
```EBNF
function ::= '(' argment-list ')' '{' (expression ';')* '}'
argment-list ::= (identifier  ','?)*
```

A function is a  lazy-evaluated group of expressions that together perform a task using arguments and return a result.

#### Structure
```EBNF
struct_key ::= identifier | '+' | '-' | '*' | '/' | '%' | '<' | '>' | '&' | '|' | '^' | '~' | '!' | (('!' | '=' | '<' | '>') > '=') |
                       x3::repeat(2)[x3::char_("+\\-<>&|")] |
                       "[]" |
                       "()"
structure ::= '[' (struct_key ':' expression ','?)* ']'
```

A structure is a composition of data.
A structure has a field that matches keys and  values.
You can access the value to specify the key with dot operator.
###### Example:
```
io = @import#c:stdio.h
struct = [
  value: 10,
  func: (arg){ <| arg + 1; }
];
io.printf("%d\n", struct.value); // 10
```

#### Scopes
```EBNF
scope ::= '{' (expression ';')* '}'
```
A scope literal is a lazy-evaluated group of expressions that doesn't take arguments.
A scope has a scope which inherits that of its declaration.
###### Example:
```
var = 10;
scope = {
  var = var + 2;
};
var = 5;
scope(); // now var is 7
```
It often used in conditional operator.

### Variable
```EBNF
variable ::= identifier
pre_variable ::= '@' > identifier;
```

Variable has a same syntax with identifier.

To declare a variable, you just have to assign to a new variable with initial value. The type will be suggested by the value.

You can use the variable's value to write the variable's name.

Also you can assign to the variable with the same way you declared that. If the right operand has a different type with the variable, it is illegal.

##### Predefined variable
pre_variable, which is a predefined variable start with '@', is used to make a special action on a compiler.
 - `@import`
  Use `@import` to import other file.
  You can specify the path to the file:
    - `#m`
      Specify the path to other scorpion file
      Returns the top level value of the file
    - `#ir`
      Specify the path to llvm ir file
      Returns a structure contains all function declarations in the file
    - `#c`
      Specify the c header
      Returns a structure contains all function declarations in the file
  - `@self`
    This value refers the function that you're in.
    `(){ @self(); }() // infinite loop`

