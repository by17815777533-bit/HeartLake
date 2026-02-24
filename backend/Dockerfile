# 固定基础镜像版本，确保构建可复现
FROM archlinux:base-20250209.0.303029 AS builder

RUN pacman -Syu --noconfirm && pacman -S --noconfirm \
    base-devel cmake git wget \
    openssl curl jsoncpp \
    postgresql-libs hiredis \
    c-ares \
    && pacman -Scc --noconfirm

# ONNX Runtime 版本与校验（sha256 需从官方 release 获取后替换）
ARG ONNX_VERSION=1.17.0
ARG ONNX_SHA256=c613cc3c80576f07194c8a4ab77b9e7fef4ef5a0e13e3f0e5e3b3e3b3e0e1239
RUN wget -q https://github.com/microsoft/onnxruntime/releases/download/v${ONNX_VERSION}/onnxruntime-linux-x64-${ONNX_VERSION}.tgz \
    && echo "${ONNX_SHA256}  onnxruntime-linux-x64-${ONNX_VERSION}.tgz" | sha256sum -c - \
    && tar xzf onnxruntime-linux-x64-${ONNX_VERSION}.tgz \
    && cp -r onnxruntime-linux-x64-${ONNX_VERSION}/include/* /usr/local/include/ \
    && cp -r onnxruntime-linux-x64-${ONNX_VERSION}/lib/* /usr/local/lib/ \
    && ldconfig \
    && rm -rf onnxruntime-linux-x64-${ONNX_VERSION}*

# Install Drogon
RUN git clone --depth 1 --branch v1.9.11 https://github.com/drogonframework/drogon.git /tmp/drogon && \
    cd /tmp/drogon && git submodule update --init && \
    mkdir build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_EXAMPLES=OFF -DBUILD_CTL=OFF -DBUILD_ORM=OFF && \
    make -j$(nproc) && make install && \
    rm -rf /tmp/drogon

WORKDIR /app
COPY include/ include/
COPY src/ src/
COPY cmake/ cmake/
COPY CMakeLists.txt .

RUN mkdir build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release -DHEARTLAKE_USE_ONNX=ON && \
    make -j$(nproc)

# 固定基础镜像版本，确保运行时可复现
FROM archlinux:base-20250209.0.303029

RUN pacman -Syu --noconfirm && pacman -S --noconfirm \
    openssl curl jsoncpp postgresql-libs postgresql hiredis c-ares \
    && pacman -Scc --noconfirm

RUN useradd -m -s /bin/bash heartlake

WORKDIR /app
COPY --from=builder /app/build/HeartLake .
COPY --from=builder /usr/local/lib/libonnxruntime* /usr/local/lib/
RUN ldconfig
COPY migrations/ migrations/
COPY start-docker.sh .
RUN chmod +x start-docker.sh && \
    mkdir -p logs uploads public models && \
    chown -R heartlake:heartlake /app

USER heartlake
EXPOSE 8080

CMD ["./start-docker.sh"]
