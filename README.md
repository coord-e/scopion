![scopion](https://raw.githubusercontent.com/wiki/coord-e/scopion/scopion-with-margin.svg?sanitize=true)

[![Travis](https://img.shields.io/travis/coord-e/scopion.svg?style=flat-square)](https://travis-ci.org/coord-e/scopion) [![Docker Automated buil](https://img.shields.io/docker/automated/coorde/scopion.svg?style=flat-square)](https://hub.docker.com/r/coorde/scopion/) [![Docker Build Statu](https://img.shields.io/docker/build/coorde/scopion.svg?style=flat-square)](https://hub.docker.com/r/coorde/scopion/)
[![license](https://img.shields.io/github/license/coord-e/scopion.svg?style=flat-square)](COPYING) [![GitHub release](https://img.shields.io/github/release/coord-e/scopion.svg?style=flat-square)](https://github.com/coord-e/scopion/releases) [![C++ Standard](https://img.shields.io/badge/C%2B%2B-14-yellow.svg?style=flat-square)](https://isocpp.org/wiki/faq/cpp14)

a statically-typed functional programming language with powerful objective syntax

Try now: [scopion.coord-e.com/try](https://scopion.coord-e.com/try)

###### Example:

```
(argc, argv){
  io = @import#c:stdio.h; // Import C header directly!

  v1 = v2 = [
    real: 10,
    add: (val, self){
      newv = self;
      newv.real = self.real + val;
      |> newv;
    },
    +: (rhs, self){ // Operator overload
      |> self.:add(rhs.real);
    },
  ];

  v1.=add(1); // Object-oriented call
  io.printf("v1.real => %d\n", v1.real);

  io.printf("(v1+v2).real => %d\n", (v1+v2).real);
}
```

###### Output:

```
v1.real => 11
(v1+v2).real => 21
```

**This project is heavily under development.**

**scopion has a serious problem around typesystem and currently impractical very much. This will be fixed in v0.0.4**

# Why scopion?
- **Intuitive**
  - Removal of 'global'
  - Just right fusion of Functional and Object-oriented programming
  - Referentially transparent function
  - Powerful objective syntax
- **Concise**
  - No reserved words
  - Powerful type suggestion
- **Fast**
  - Statically typed compiler
  - Optimization and native code generation by LLVM
- **C-friendly**
  - Directly use of C function

All features are listed in [docs/Feature.md](docs/Feature.md)

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

      -h, --help                        Print this help
      -t[type], --type=[type]           Specify the type of output
                                        One of: asm, ast, ir, obj
      -o[path], --output=[path]         Specify the output path
                                        Default: ./a.out
      -a[triple], --arch=[triple]       Specify the target triple
                                        Default: native
      -O[level], --optimize=[level]     Set optimization level (0-3)
                                        Default: 3
      -V, --version                     Print version
      filename                          File to compile
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
cmake .. -DCMAKE_BUILD_TYPE=Release -DFORMAT_BEFORE_BUILD=OFF
make -j"$(nproc)" # build
sudo make install # install
```

# License
This program is licensed by GPL v3. See `COPYING`.
