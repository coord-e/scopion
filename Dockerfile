From debian:stretch

COPY . scopion/

RUN apt-get update \
    && apt-get install -y --no-install-recommends wget ca-certificates gnupg \
    && cp /etc/apt/sources.list /etc/apt/sources.list.bak \
    && echo "deb http://apt.llvm.org/stretch/ llvm-toolchain-stretch-5.0 main" >> /etc/apt/sources.list \
    && wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | apt-key add - \
    && apt-get update \
    && apt-get -y install --no-install-recommends clang-5.0 llvm-5.0 libgc-dev exuberant-ctags build-essential git libclang-5.0-dev libclang1-5.0 libllvm5.0 llvm-5.0-dev libedit-dev libz-dev llvm-5.0-runtime clang-format-5.0 clang-tidy-5.0 \
    && update-alternatives --install /usr/local/bin/clang clang `which clang-5.0` 10 \
    && update-alternatives --install /usr/local/bin/clang++ clang++ `which clang++-5.0` 10 \
    && update-alternatives --install /usr/local/bin/llc llc `which llc-5.0` 10 \
    && update-alternatives --install /usr/local/bin/clang-format clang-format `which clang-format-5.0` 10 \
    && update-alternatives --install /usr/local/bin/clang-tidy clang-tidy `which clang-tidy-5.0` 10 \
    && wget https://dl.bintray.com/boostorg/release/1.64.0/source/boost_1_64_0.tar.gz \
    && tar xf boost_1_64_0.tar.gz \
    && cd boost_1_64_0/ \
    && ./bootstrap.sh \
    && ./b2 install --with-filesystem -j"$(nproc)" \
    && cd .. \
    && rm -rf boost_1_64_0/ boost_1_64_0.tar.gz \
    && wget https://cmake.org/files/v3.8/cmake-3.8.2-Linux-x86_64.tar.gz \
    && tar xf cmake-3.8.2-Linux-x86_64.tar.gz \
    && cp cmake-3.8.2-Linux-x86_64/bin/* /usr/local/bin/ \
    && cp -r cmake-3.8.2-Linux-x86_64/share/* /usr/local/share/ \
    && rm -rf cmake-3.8.2-Linux-x86_64/ cmake-3.8.2-Linux-x86_64.tar.gz \
    && mkdir -p scopion/build \
    && cd scopion/build \
    && cmake .. -DCMAKE_BUILD_TYPE=Release -DFORMAT_BEFORE_BUILD=OFF -DWITHOUT_TEST=ON \
    && make -j"$(nproc)" \
    && make install \
    && cd ../.. \
    && rm -rf scopion/ \
    && apt-get -y remove wget gnupg ca-certificates build-essential git libclang-5.0-dev llvm-5.0-dev libedit-dev libz-dev clang-format-5.0 clang-tidy-5.0 \
    && mv /etc/apt/sources.list.bak /etc/apt/sources.list \
    && apt-get update \
    && apt-get autoremove -y \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/* \
    && ldconfig \
    && scopc -V

CMD ["bash"]
