# HeartLake 后端 — 多阶段构建
# 阶段1: 编译环境
FROM ubuntu:24.04 AS builder

RUN apt-get update && apt-get install -y --no-install-recommends \
        cmake g++ git ca-certificates \
        libssl-dev libcurl4-openssl-dev libjsoncpp-dev \
        libpq-dev libhiredis-dev libc-ares-dev uuid-dev \
    && rm -rf /var/lib/apt/lists/*

# 编译 Drogon
ARG DROGON_VERSION=v1.9.11
RUN git clone --depth 1 --branch ${DROGON_VERSION} \
        https://github.com/drogonframework/drogon.git /tmp/drogon \
    && cd /tmp/drogon && git submodule update --init \
    && mkdir build && cd build \
    && cmake .. -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_EXAMPLES=OFF -DBUILD_CTL=OFF -DBUILD_ORM=OFF \
    && make -j"$(nproc)" && make install \
    && rm -rf /tmp/drogon

WORKDIR /src
COPY . .

RUN mkdir -p build && cd build \
    && cmake .. -DCMAKE_BUILD_TYPE=Release \
    && make -j"$(nproc)"

# 阶段2: 最小运行时镜像
FROM ubuntu:24.04 AS runtime

RUN apt-get update && apt-get install -y --no-install-recommends \
        libssl3t64 libcurl4t64 libjsoncpp25 \
        libpq5 libhiredis1.1.0 libc-ares2 \
        ca-certificates tzdata curl postgresql-client \
    && rm -rf /var/lib/apt/lists/*

RUN groupadd -r heartlake && useradd -r -g heartlake -s /sbin/nologin heartlake

WORKDIR /app

COPY --from=builder /src/build/HeartLake .
COPY --from=builder /src/config.json.example ./config.json
COPY --from=builder /src/start-docker.sh ./start-docker.sh
COPY --from=builder /src/migrations ./migrations

RUN chown -R heartlake:heartlake /app
RUN chmod +x /app/start-docker.sh

USER heartlake

EXPOSE 8080

HEALTHCHECK --interval=30s --timeout=5s --start-period=10s --retries=3 \
    CMD curl -sf http://localhost:8080/api/health || exit 1

CMD ["/bin/bash", "/app/start-docker.sh"]
