From debian:stretch

COPY . scopion/

RUN apt-get update \
    && apt-get install -y --no-install-recommends wget ca-certificates gnupg \
    && cp /etc/apt/sources.list /etc/apt/sources.list.bak \
    && echo "deb http://apt.llvm.org/stretch/ llvm-toolchain-stretch-5.0 main" >> /etc/apt/sources.list \
    && wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | apt-key add - \
    && apt-get update \
    && apt-get -y install --no-install-recommends clang-5.0 llvm-5.0 libgc-dev exuberant-ctags build-essential git libboost-dev libboost-filesystem-dev cmake libclang-5.0-dev libclang1-5.0 libllvm5.0 llvm-5.0-dev libedit-dev libz-dev llvm-5.0-runtime clang-format-5.0 clang-tidy-5.0 \
    && update-alternatives --install /usr/local/bin/clang clang `which clang-5.0` 10 \
    && update-alternatives --install /usr/local/bin/clang++ clang++ `which clang++-5.0` 10 \
    && update-alternatives --install /usr/local/bin/llc llc `which llc-5.0` 10 \
    && update-alternatives --install /usr/local/bin/clang-format clang-format `which clang-format-5.0` 10 \
    && update-alternatives --install /usr/local/bin/clang-tidy clang-tidy `which clang-tidy-5.0` 10 \
    && mkdir -p scopion/build \
    && cd scopion/build \
    && cmake .. -DCMAKE_CXX_COMPILER=clang++ -DCONFIG=RELEASE -DFORMAT_BEFORE_BUILD=OFF \
    && make -j"$(nproc)" \
    && make test \
    && make install \
    && cd ../.. \
    && rm -rf scopion/ \
    && apt-get -y remove wget gnupg ca-certificates build-essential git cmake libclang-5.0-dev llvm-5.0-dev libedit-dev libz-dev clang-format-5.0 clang-tidy-5.0 \
    && mv /etc/apt/sources.list.bak /etc/apt/sources.list \
    && apt-get update \
    && apt-get autoremove -y \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/* \
    && ldconfig \
    && scopc -V

CMD ["bash"]
