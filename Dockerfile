# === Build Stage ===
FROM alpine:edge AS build

RUN apk update && \
    apk add --no-cache \
        build-base \
        cmake \
        boost-dev \
        git

WORKDIR /app
COPY . .

RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && \
    cmake --build build --parallel && \
    strip /app/build/kut

# === Runtime Stage ===
FROM alpine:edge AS runtime

RUN apk add --no-cache \
    libstdc++ \
    boost

WORKDIR /app
COPY --from=build /app/build/kut /app/build/kut

ENTRYPOINT ["/app/build/kut"]
CMD ["-h"]