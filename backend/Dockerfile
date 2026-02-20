FROM archlinux:latest AS builder

RUN pacman -Syu --noconfirm && pacman -S --noconfirm \
    base-devel cmake git \
    openssl curl jsoncpp \
    postgresql-libs hiredis \
    gtest c-ares \
    && pacman -Scc --noconfirm

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
COPY tests/ tests/
COPY CMakeLists.txt .

RUN mkdir build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release && \
    make -j$(nproc)

# Runtime
FROM archlinux:latest

RUN pacman -Syu --noconfirm && pacman -S --noconfirm \
    openssl curl jsoncpp postgresql-libs postgresql hiredis c-ares \
    && pacman -Scc --noconfirm

RUN useradd -m -s /bin/bash heartlake

WORKDIR /app
COPY --from=builder /app/build/HeartLake .
COPY migrations/ migrations/
COPY start-docker.sh .
RUN chmod +x start-docker.sh && \
    mkdir -p logs uploads public && \
    chown -R heartlake:heartlake /app

USER heartlake
EXPOSE 8080

CMD ["./start-docker.sh"]
