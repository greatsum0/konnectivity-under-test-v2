# Build
FROM alpine:edge AS build

RUN apk add --no-cache build-base cmake git linux-headers libpcap-dev

WORKDIR /app
COPY . ./

RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
 && cmake --build build --parallel \
 && strip /app/build/kut


FROM alpine:edge AS runtime

RUN apk add --no-cache libstdc++ libpcap

WORKDIR /app
COPY --from=build /app/build/kut /app/build/kut

ENTRYPOINT ["/app/build/kut"]
CMD ["-h"]
