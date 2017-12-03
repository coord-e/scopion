# Operators

**This document is written in progress.**
See [Parser.cpp](lib/parser/parser.cpp) for full syntax definition.

| Precedence | Operator Group                                             | Symbol                      | Associativity | Customizability |
| ----------:| ---------------------------------------------------------- | --------------------------- | ------------- | ---------------- |
|         18 | Grouping                                                   | `()`                        | N/A           | No               |
|         17 | [Dot](#dot-operator)                                       | `.` `.:` `.=`               | left-to-right | No               |
|         16 | [Attribute](#attribute-expression)                         | `#`                         | left-to-right | No               |
|         15 | [Call](#call-operator) / [Index](#index-operator)          | `()` `[]`                   | left-to-right | Yes              |
|         14 | [Post single operator](#single-operators)                  | `++` `--`                   | N/A           | Yes              |
|         13 | [Pre single operator](#single-operators)                   | `!` `~` `++` `--`           | left-to-right | Yes              |
|         12 | [Exponentiation](#arithmetic-operators)                    | `**`                        | left-to-right | Yes              |
|         11 | [Multiplication / Division](#arithmetic-operators)         | `*` `/`                     | left-to-right | Yes              |
|         10 | [Addition / Subtraction / Reminder](#arithmetic-operators) | `+` `-` `%`                 | left-to-right | Yes              |
|          9 | [Shift](#arithmetic-operators)                             | `>>` `<<`                   | left-to-right | Yes              |
|          8 | [Comparison](#arithmetic-operators)                        | `>` `<` `>=` `<=` `==` `!=` | left-to-right | Yes              |
|          7 | [And](#arithmetic-operators)                               | `&`                         | left-to-right | Yes              |
|          6 | [Xor](#arithmetic-operators)                               | `^`                         | left-to-right | Yes              |
|          5 | [Or](#arithmetic-operators)                                | <code>&#124;</code>         | left-to-right | Yes              |
|          4 | [Logical And](#arithmetic-operators)                       | `&&`                        | left-to-right | Yes              |
|          3 | [Logical Or](#arithmetic-operators)                        | <code>&#124;&#124;</code>   | left-to-right | Yes              |
|          2 | [Conditional](#conditional-operator)                       | `?:`                        | right-to-left | No               |
|          1 | [Assignment](#assignment-operator)                         | `=`                         | right-to-left | No               |
|          0 | [Return](#return-operator)                                 | <code>&#124;></code>        | left-to-right | No               |

## Dot operator
```EBNF
dot_expr ::= primary (".:" struct_key |
                      ".=" struct_key |
                      "." struct_key)*
```
Use dot operator to extract value from the [structure](docs/Literal.md#structure) using specified key.

### Objective dot operator: `.:`
```
a.:b(c)
```
has the same meaning as:
```
a.b(c, a)
```

### Objective dot assignment operator: `.=`
```
a.=b(c)
```
has the same meaning as:
```
a = a.:b(c)
```

**Objective dot operators (`.:`, `.=`) cannot be used without a call operator.**

## Attribute expression

```EBNF
attr_expr ::= dot_expr ("#" identifier (":" attribute_val)- )*;
```

Attribute is used to tell the compiler information about the value. e.g. type

## Brackets operators
```EBNF
call_expr ::= attr_expr (("(" (expression ","- )* ")") |
                         ("[" expression "]"))*;
```

### Call operator
Use call operator to call functions.
You can supply 0 or more arguments to functions to call.
When the callee is a function, it simply 'call' that.

### Index operator
Use Index operator to extract value from an indexed-container like an array.
If this is used with pointers, this operator acts the same as C's [] operator.
e.g. `argv[N] // Nth command line argument`

## Single operators
```EBNF
post_sinop_expr ::= (call_expr "++") |
                    (call_expr "--") |
                    call_expr;

pre_sinop_expr ::= post_sinop_expr |
                    ("!" post_sinop_expr) |
                    ("~" post_sinop_expr) |
                    ("++" post_sinop_expr) |
                    ("--" post_sinop_expr);
```
**There is no difference between pre-increment and post-increment**

## Arithmetic operators
```EBNF
pow_expr = pre_sinop_expr ("**" pre_sinop_expr)*;

mul_expr = pow_expr (("*" pow_expr) |
                     ("/" pow_expr))*;

add_expr = mul_expr (("+" mul_expr) |
                     ("-" mul_expr) |
                     ("%" mul_expr))*;

shift_expr = add_expr ((">>" add_expr) |
                       ("<<" add_expr));

cmp_expr = shift_expr (((">" - ">=") shift_expr) |
                       (("<" - "<=") shift_expr) |
                       (">=" shift_expr) |
                       ("<=" shift_expr) |
                       ("==" shift_expr) |
                       ("!=" shift_expr))*;

iand_expr = cmp_expr (("&" - "&&") cmp_expr)*;

ixor_expr = iand_expr ("^" iand_expr)*;

ior_expr = ixor_expr (("|" - "||") ixor_expr)*;

land_expr = ior_expr ("&&" ior_expr)*;

lor_expr = land_expr ("||" land_expr)*;
```

## Conditional operator
```EBNF
cond_expr = lor_expr ("?" lor_expr ":" lor_expr)*;
```

### With values

Same as 'ternary operator' in the other language.

###### Example:
```
val = false ? 1 : 0; // val is 0
```

### With scopes

You can use a conditonal operator like `if` when you use it with scopes.
###### Example:
```
(){
  true ? {
    |> 0;
  } : {
    |> 5;
  }
} // return 0
```


## Assignment operator
```EBNF
assign_expr = cond_expr "=" assign_expr | cond_expr;
```
Assign a value to variables.
If the lhs variable has not defined yet, it'll be defined.

An assignment operator has right-to-left associativity, so you can write like this:
```
a = b = 0; // both a and b is 0
```

## Return operator
```EBNF
ret_expr = assign_expr | ("|>" assign_expr);
```

Return a value from current function.
