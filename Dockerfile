# ─── Build stage ──────────────────────────────────────────────────────────────
FROM alpine:edge AS build

# Install compilers, CMake, and all the -dev packages we need
RUN apk add --no-cache \
      build-base \
      cmake \
      git \
      linux-headers \
      libpcap-dev \
      boost-dev \
      yaml-cpp-dev

WORKDIR /app
COPY . .

# Configure & compile
RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
 && cmake --build build --parallel \
 && strip build/kut

# ─── Runtime stage ───────────────────────────────────────────────────────────
FROM alpine:edge AS runtime

# Only pull in the shared runtime libs
RUN apk add --no-cache \
      libstdc++ \
      libpcap \
      boost \
      yaml-cpp

WORKDIR /app
COPY --from=build /app/build/kut /app/build/kut

ENTRYPOINT ["/app/build/kut"]
CMD ["-h"]
