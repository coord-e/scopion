From debian:stretch

RUN apt-get update && apt-get install -y wget gnupg

RUN echo "deb http://apt.llvm.org/stretch/ llvm-toolchain-stretch-4.0 main" >> /etc/apt/sources.list
RUN wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | apt-key add -

RUN apt-get update && apt-get -y upgrade

RUN apt-get -y install clang-4.0 llvm-4.0 libgc-dev exuberant-ctags build-essential libclang-4.0-dev libclang1-4.0 libllvm4.0 llvm-4.0-dev libedit-dev libz-dev llvm-4.0-runtime clang-format-4.0 clang-tidy-4.0
RUN apt-get -y install git

RUN update-alternatives --install /usr/local/bin/clang clang `which clang-4.0` 10
RUN update-alternatives --install /usr/local/bin/clang++ clang++ `which clang++-4.0` 10
RUN update-alternatives --install /usr/local/bin/llc llc `which llc-4.0` 10
RUN update-alternatives --install /usr/local/bin/clang-format clang-format `which clang-format-4.0` 10
RUN update-alternatives --install /usr/local/bin/clang-tidy clang-tidy `which clang-tidy-4.0` 10

WORKDIR wd_

RUN wget https://dl.bintray.com/boostorg/release/1.64.0/source/boost_1_64_0.tar.bz2
RUN tar xf boost_1_64_0.tar.bz2
RUN cp -r boost_1_64_0/boost /usr/local/include

RUN wget https://cmake.org/files/v3.8/cmake-3.8.2-Linux-x86_64.tar.gz
RUN tar xf cmake-3.8.2-Linux-x86_64.tar.gz
RUN cp cmake-3.8.2-Linux-x86_64/bin/* /usr/local/bin/
RUN cp -r cmake-3.8.2-Linux-x86_64/share/* /usr/local/share/

RUN git clone --depth 1 -b develop https://github.com/coord-e/scopion

WORKDIR scopion/build

RUN cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=clang++ -DENABLE_CLANG_TIDY=OFF -DFORMAT_BEFORE_BUILD=OFF
RUN make -j"$(nproc)" && make install

WORKDIR /
RUN ldconfig
RUN rm -rf wd_

RUN scopc -V
