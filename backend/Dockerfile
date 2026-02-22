FROM archlinux:latest AS builder

RUN pacman -Syu --noconfirm && pacman -S --noconfirm \
    base-devel cmake git wget \
    openssl curl jsoncpp \
    postgresql-libs hiredis \
    c-ares \
    && pacman -Scc --noconfirm

# Install ONNX Runtime
RUN wget -q https://github.com/microsoft/onnxruntime/releases/download/v1.17.0/onnxruntime-linux-x64-1.17.0.tgz \
    && tar xzf onnxruntime-linux-x64-1.17.0.tgz \
    && cp -r onnxruntime-linux-x64-1.17.0/include/* /usr/local/include/ \
    && cp -r onnxruntime-linux-x64-1.17.0/lib/* /usr/local/lib/ \
    && ldconfig \
    && rm -rf onnxruntime-linux-x64-1.17.0*

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

# Runtime
FROM archlinux:latest

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
