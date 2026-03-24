# HeartLake 后端 — 多阶段构建
# 阶段1: 编译环境
FROM ubuntu:24.04 AS builder

ARG BUILD_JOBS=1
ARG ONNX_RUNTIME_ARCHIVE=third_party/onnxruntime-linux-x64-1.22.0.tgz

RUN apt-get update && apt-get install -y --no-install-recommends \
        cmake make g++ ca-certificates \
        libdrogon-dev \
        libssl-dev libcurl4-openssl-dev libjsoncpp-dev zlib1g-dev libsqlite3-dev \
        libyaml-cpp-dev libbrotli-dev \
        default-libmysqlclient-dev \
        libpq-dev libhiredis-dev libc-ares-dev uuid-dev \
    && rm -rf /var/lib/apt/lists/*

# Drogon 直接使用 Ubuntu 官方包，避免构建期依赖外网源码仓库

WORKDIR /src
COPY . .

RUN test -f "${ONNX_RUNTIME_ARCHIVE}" \
    && mkdir -p /src/third_party \
    && tar -xzf "${ONNX_RUNTIME_ARCHIVE}" -C /src/third_party

RUN mkdir -p build && cd build \
    && cmake .. -DCMAKE_BUILD_TYPE=Release \
        -DHEARTLAKE_BUILD_TESTS=OFF \
        -DHEARTLAKE_USE_ONNX=ON \
        -DOnnxRuntime_ROOT=/src/third_party/onnxruntime-linux-x64-1.22.0 \
        -DMYSQL_INCLUDE_DIRS=/usr/include/mysql \
        -DMYSQL_LIBRARIES=/usr/lib/x86_64-linux-gnu/libmysqlclient.so \
    && make -j"${BUILD_JOBS}"

# 阶段2: 最小运行时镜像
FROM ubuntu:24.04 AS runtime

RUN apt-get update && apt-get install -y --no-install-recommends \
        libdrogon1t64 \
        libssl3t64 libcurl4t64 libjsoncpp25 \
        libpq5 libhiredis1.1.0 libc-ares2 \
        ca-certificates tzdata curl postgresql-client \
    && rm -rf /var/lib/apt/lists/*

RUN groupadd -r heartlake && useradd -r -g heartlake -s /sbin/nologin heartlake

WORKDIR /app

COPY --from=builder /src/build/HeartLake .
COPY --from=builder /src/start-docker.sh ./start-docker.sh
COPY --from=builder /src/migrations ./migrations
COPY --from=builder /src/models ./models
COPY --from=builder /src/third_party/onnxruntime-linux-x64-1.22.0 ./third_party/onnxruntime-linux-x64-1.22.0

RUN mkdir -p /app/logs \
    && chown -R heartlake:heartlake /app
RUN chmod +x /app/start-docker.sh

USER heartlake

EXPOSE 8080

HEALTHCHECK --interval=30s --timeout=5s --start-period=10s --retries=3 \
    CMD curl -sf http://localhost:8080/api/health || exit 1

CMD ["/bin/bash", "/app/start-docker.sh"]
