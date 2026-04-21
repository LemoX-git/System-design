FROM ghcr.io/userver-framework/ubuntu-24.04-userver:v2.14

WORKDIR /app

COPY . .

RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && \
    cmake --build build -j && \
    useradd -m -u 10001 appuser && \
    chown -R appuser:appuser /app

USER appuser

EXPOSE 8080

CMD ["./build/recipe_service","--config", "configs/config.yaml", "--config_vars", "configs/config_vars.yaml"]
