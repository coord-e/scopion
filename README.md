<div style="text-align:center"><img src ="https://raw.githubusercontent.com/wiki/coord-e/scopion/scopion.png" /></div>

[![Travis](https://img.shields.io/travis/coord-e/scopion.svg?style=flat-square)](https://travis-ci.org/coord-e/scopion) [![Docker Automated buil](https://img.shields.io/docker/automated/coorde/scopion.svg?style=flat-square)](https://hub.docker.com/r/coorde/scopion/) [![Docker Build Statu](https://img.shields.io/docker/build/coorde/scopion.svg?style=flat-square)](https://hub.docker.com/r/coorde/scopion/)
[![license](https://img.shields.io/github/license/coord-e/scopion.svg?style=flat-square)](COPYING) [![GitHub release](https://img.shields.io/github/release/coord-e/scopion.svg?style=flat-square)](https://github.com/coord-e/scopion/releases)
# scopion
a statically-typed functional programming language with powerful objective syntax

Try now: [scopion.coord-e.com/try](https://scopion.coord-e.com/try)

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
- no such thing like 'global'
- almost all functions has a referentially transparency
- support powerful objective syntax
- no reserved words
- statically typed
- types are suggested in almost scene
- Embedded garbage collection
- Optimization and native code generation by LLVM

# Getting started
## Prerequirements
- llvm, clang (v5.0.0~)
- Boost.Filesystem (v1.62~)
- libgc
- ctags
## Supported Platforms
- macOS
- GNU/Linux

Only x86_64 is currently supported.

## Installation

If you are in Ubuntu 17.04~, Debian stretch~, or macOS Sierra~, just paste this at a terminal prompt:
```shell
curl -fsSL https://scopion.coord-e.com/get | bash
```

### Binary downloads
- [Debian/Ubuntu](https://github.com/coord-e/scopion/releases/download/v0.0.3.1/scopion_0.0.3.1-Linux_x86_64.deb)
- [Other GNU/Linux](https://github.com/coord-e/scopion/releases/download/v0.0.3.1/scopion_0.0.3.1-Linux_x86_64.tar.bz2)
- [Darwin (macOS)](https://github.com/coord-e/scopion/releases/download/v0.0.3.1/scopion_0.0.3.1-Darwin_x86_64.zip)

### Docker Image
```shell
docker pull coorde/scopion
docker run -it coorde/scopion /bin/bash
```

## Enjoy
Now you can compile your scopion source
```shell
scopc prog.scc -o prog
```
### Usage
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
- (Installation prerequirements)
- Boost (v1.62~)
- cmake (v3.7~)

```shell
git clone https://github.com/coord-e/scopion
cd scopion
mkdir build && cd $_
cmake .. -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Release -DFORMAT_BEFORE_BUILD=OFF
make -j"$(nproc)" # build
sudo make install # install
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

# License
This program is licensed by GPL v3. See `COPYING`.
