FROM ubuntu:24.04 AS build
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends cmake g++ make libargon2-dev ca-certificates && rm -rf /var/lib/apt/lists/*
WORKDIR /src
COPY forge/ ./forge/
RUN cmake -S forge -B build -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build --target forge-server --parallel 2

FROM ubuntu:24.04 AS runtime
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends libargon2-1 ca-certificates curl \
    && rm -rf /var/lib/apt/lists/* \
    && useradd --system --create-home --home-dir /home/forge forge
WORKDIR /app
COPY --from=build /src/build/forge-server /usr/local/bin/forge-server
RUN mkdir -p /var/lib/forge && chown -R forge:forge /var/lib/forge
USER forge
EXPOSE 8080
VOLUME ["/var/lib/forge"]
HEALTHCHECK --interval=10s --timeout=3s --retries=5 CMD curl --fail http://127.0.0.1:8080/health || exit 1
ENTRYPOINT ["/usr/local/bin/forge-server", "--host", "0.0.0.0", "--port", "8080", "--data-dir", "/var/lib/forge"]
