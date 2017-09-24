From debian:stretch

RUN apt-get update && apt-get -y upgrade && apt-get install -y wget gnupg

RUN cp /etc/apt/sources.list /etc/apt/sources.list.bak
RUN echo "deb http://apt.llvm.org/stretch/ llvm-toolchain-stretch-5.0 main" >> /etc/apt/sources.list
RUN wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | apt-key add -

RUN apt-get update && apt-get -y install clang-5.0 llvm-5.0 libgc-dev exuberant-ctags build-essential git libclang-5.0-dev libclang1-5.0 libllvm5.0 llvm-5.0-dev libedit-dev libz-dev llvm-5.0-runtime clang-format-5.0 clang-tidy-5.0

RUN update-alternatives --install /usr/local/bin/clang clang `which clang-5.0` 10
RUN update-alternatives --install /usr/local/bin/clang++ clang++ `which clang++-5.0` 10
RUN update-alternatives --install /usr/local/bin/llc llc `which llc-5.0` 10
RUN update-alternatives --install /usr/local/bin/clang-format clang-format `which clang-format-5.0` 10
RUN update-alternatives --install /usr/local/bin/clang-tidy clang-tidy `which clang-tidy-5.0` 10

WORKDIR wd_

RUN wget https://dl.bintray.com/boostorg/release/1.64.0/source/boost_1_64_0.tar.bz2
RUN tar xf boost_1_64_0.tar.bz2
RUN cd boost_1_64_0/ && ./bootstrap.sh && ./b2 install --with-filesystem -j"$(nproc)"

RUN wget https://cmake.org/files/v3.8/cmake-3.8.2-Linux-x86_64.tar.gz
RUN tar xf cmake-3.8.2-Linux-x86_64.tar.gz
RUN cp cmake-3.8.2-Linux-x86_64/bin/* /usr/local/bin/
RUN cp -r cmake-3.8.2-Linux-x86_64/share/* /usr/local/share/

RUN git clone --depth 1 -b develop https://github.com/coord-e/scopion

WORKDIR scopion/build

RUN cmake .. -DCMAKE_BUILD_TYPE=Release -DFORMAT_BEFORE_BUILD=OFF -DWITHOUT_TEST=ON
RUN make -j"$(nproc)" && make install

WORKDIR /
RUN ldconfig
RUN scopc -V

RUN rm -rf wd_
RUN mv /etc/apt/sources.list.bak /etc/apt/sources.list
RUN apt-get update && apt-get -y remove wget gnupg build-essential git libclang-5.0-dev llvm-5.0-dev libedit-dev libz-dev clang-format-5.0 clang-tidy-5.0
