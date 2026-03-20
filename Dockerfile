# --- Этап 1: Сборка ---
FROM ghcr.io/userver-framework/ubuntu-24.04-userver:latest AS builder

WORKDIR /build

COPY CMakeLists.txt ./
COPY src/ ./src/

RUN mkdir -p build && \
    cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release && \
    make -j$(nproc)

# --- Этап 2: Рантайм ---
FROM ghcr.io/userver-framework/ubuntu-24.04-userver:latest

RUN useradd -m -s /bin/bash appuser

WORKDIR /app

COPY --from=builder /build/build/a.out ./a.out

RUN mkdir -p /app/configs
COPY ./configs/static_config.yaml ./configs/static_config.yaml
COPY ./configs/openapi.yaml ./configs/openapi.yaml

RUN chown -R appuser:appuser /app

USER appuser

EXPOSE 8080

HEALTHCHECK --interval=30s --timeout=10s --start-period=5s --retries=5 \
    CMD curl -f http://localhost:8080/api/v1/ping || exit 1

CMD ["./a.out", "-c", "./configs/static_config.yaml"]