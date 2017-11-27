<div style="text-align:center"><img src ="https://raw.githubusercontent.com/wiki/coord-e/scopion/scopion.png" /></div>

[![Travis](https://img.shields.io/travis/coord-e/scopion.svg)](https://travis-ci.org/coord-e/scopion) [![Docker Automated buil](https://img.shields.io/docker/automated/coorde/scopion.svg)](https://hub.docker.com/r/coorde/scopion/) [![Docker Build Statu](https://img.shields.io/docker/build/coorde/scopion.svg)](https://hub.docker.com/r/coorde/scopion/)
[![license](https://img.shields.io/github/license/coord-e/scopion.svg)](COPYING) [![GitHub release](https://img.shields.io/github/release/coord-e/scopion.svg)](https://github.com/coord-e/scopion/releases)
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

# License
This program is licensed by GPL v3. See `COPYING`.
