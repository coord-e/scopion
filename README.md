<div style="text-align:center"><img src ="https://raw.githubusercontent.com/wiki/coord-e/scopion/scopion.png" /></div>


# scopion
scopion is a new programming language with simple syntax.

###### Example:
```
(argc, argv){
  io = @import#c:stdio.h;

  v1 = v2 = [
    real: 10,
    add: (val, self){
      newv = self;
      newv.real = self.real + val;
      |> newv;
    },
    +: (rhs, self){
      |> self.:add(rhs.real);
    },
  ];

  v1.=add(1);
  io.printf("v1.real => %d\n", v1.real); // 11

  io.printf("(v1+v2).real => %d\n", (v1+v2).real); // 21
}
```

###### Output:
```
v1.real => 11
(v1+v2).real => 21
```

**This project is heavily under development.**

# Features
- no global
- referentially transparent (without c functions)
- objective syntax
- no reserved words
- statically typed
- types are suggested in almost scene
- Embedded garbage collection

# Getting started
## Prerequirements
- llvm, clang (v5.0.0~)
- libgc
- ctags
## Installation

### Binary downloads
- [Debian/Ubuntu](https://github.com/coord-e/scopion/releases/download/v0.0.2/scopion_0.0.2-Linux_x86_64.deb)
- [Other GNU/Linux](https://github.com/coord-e/scopion/releases/download/v0.0.2/scopion_0.0.2-Linux_x86_64.tar.bz2)
- [Darwin (macOS)](https://github.com/coord-e/scopion/releases/download/v0.0.2/scopion_0.0.2-Darwin_x86_64.zip)

Or you can [build from source](#build-from-source)


## Enjoy
You can compile your program with
```shell
scopc prog.scc -o prog
```
Usage:
```shell
usage: scopc [options] ... filename ...
options:
  -t, --type        Specify the type of output (ir, ast, asm, obj) (string [=obj])
  -o, --output      Specify the output path (string [=./a.out])
  -a, --arch        Specify the target triple (string [=native])
  -O, --optimize    Enable optimization (1-3) (int [=1])
  -h, --help        Print this help
  -V, --version     Print version
```

## Build from source
if there is no suitable prebuilt binary for your environment, you can build scopion from source.
### Prerequirements
- Boost (v1.64~)
```shell
# clone this repo
git clone https://github.com/coord-e/scopion
# make build directory and run cmake
cd scopion
mkdir build && cd $_
cmake .. -DCMAKE_CXX_COMPILER=clang++ # clang++ >=4.0 is required 
#build and install (this may takes time)
make -j 4 && make install
```

# TODO
- [ ] Add package management system
- [ ] Varadic array and structure
- [ ] Associative array
- [ ] Logging
- [ ] Conpile-time execution of functions
- [ ] Cast operator
- [ ] Coroutine
- [ ] Add more features to stdlib
- [ ] Tuple (array with different types)

# Syntax
**This document is being written.**
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
|----------|-----------------|------|
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

# License
This program is licensed by GPL v3. See `COPYING`.
