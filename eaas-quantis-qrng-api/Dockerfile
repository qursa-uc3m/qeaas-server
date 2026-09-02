FROM ubuntu:24.04 AS builder

ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential cmake libdrogon-dev libjsoncpp-dev libusb-1.0-0-dev pkg-config \
    && rm -rf /var/lib/apt/lists/*

COPY dependencies/quantis-libraries /tmp/quantis-libraries
RUN /tmp/quantis-libraries/install_quantis_libraries.sh \
      --target /opt/quantis \
      --library-path /tmp/quantis-libraries \
      --device usb \
      --extraction \
      --skip-dependencies

WORKDIR /src
COPY CMakeLists.txt main.cc RandomSource.cc RandomSource.h ./
COPY controllers ./controllers
ARG OS_XOR=ON
RUN cmake -S . -B build \
      -DCMAKE_BUILD_TYPE=Release \
      -DENABLE_OS_XOR="${OS_XOR}" \
      -DQUANTIS_ROOT=/opt/quantis \
    && cmake --build build --parallel

FROM ubuntu:24.04

ARG DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
    libdrogon1t64 libusb-1.0-0 \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /opt/quantis/lib/ /opt/quantis/lib/
COPY --from=builder /opt/quantis/share/ /opt/quantis/share/
COPY --from=builder /src/build/eaas-quantis-qrng-api /usr/local/bin/eaas-quantis-qrng-api

RUN echo '/opt/quantis/lib' > /etc/ld.so.conf.d/quantis.conf && ldconfig

EXPOSE 6065
ENTRYPOINT ["/usr/local/bin/eaas-quantis-qrng-api"]
CMD ["--source", "pcie", "--fallback", "on"]
