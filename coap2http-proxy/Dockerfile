FROM ubuntu:latest

# proxy path
ARG PROXYDIR="/home/coap2http-proxy"
ENV CERTS_PATH="${PROXYDIR}/certs"

# ── PQC / wolfSSL build knobs ──────────────────────────────────
# PQC_BACKEND:     builtin (default) | liboqs | none
# WOLFSSL_VERSION: git tag, e.g. v5.8.2-stable
# WOLFSSL_GROUPS:  colon-separated TLS 1.3 named groups for libcoap
ARG PQC_BACKEND=builtin
ARG WOLFSSL_VERSION=v5.8.2-stable
ARG WOLFSSL_GROUPS="P-256:P-384:X25519:ML_KEM_512:ML_KEM_768:ML_KEM_1024"

# Non-interactive mode for Docker build
ENV DEBIAN_FRONTEND=noninteractive

# get all software packages required
RUN apt update && apt upgrade -y && \
    apt install -y git gcc g++ cmake libjsoncpp-dev uuid-dev zlib1g-dev openssl libssl-dev libcurl4-openssl-dev

WORKDIR ${PROXYDIR}

# copy all files
COPY . ${PROXYDIR}

# Verify the copy of files
RUN ls -la ${PROXYDIR} && ls -la ${PROXYDIR}/coap2http-proxy

# Set execute permissions on the scripts
RUN chmod +x ${PROXYDIR}/coap2http-proxy/install_*.sh

# Patch scripts for non-interactive Docker build:
# 1. Remove interactive 'read' prompts from liboqs installer
# 2. Add -y flag to apt-get in proxy installer
RUN sed -i '/read -p/,/^fi$/d' ${PROXYDIR}/coap2http-proxy/install_liboqs_for_wolfssl.sh && \
    sed -i 's/apt-get install /apt-get install -y /g' ${PROXYDIR}/coap2http-proxy/install_proxy.sh

# Conditionally install liboqs (only when PQC_BACKEND=liboqs)
RUN if [ "${PQC_BACKEND}" = "liboqs" ]; then \
        ${PROXYDIR}/coap2http-proxy/install_liboqs_for_wolfssl.sh; \
    else \
        echo ">>> Skipping liboqs (PQC_BACKEND=${PQC_BACKEND})"; \
    fi

# Build wolfSSL (reads PQC_BACKEND + WOLFSSL_VERSION env vars)
RUN PQC_BACKEND=${PQC_BACKEND} WOLFSSL_VERSION=${WOLFSSL_VERSION} \
    ${PROXYDIR}/coap2http-proxy/install_wolfssl.sh && \
    ldconfig

# Create WolfSSL cmake config (autotools doesn't generate one, but libcoap needs it)
RUN mkdir -p /usr/local/lib/cmake/wolfssl && \
    echo 'set(WolfSSL_FOUND TRUE)'                                       >  /usr/local/lib/cmake/wolfssl/wolfssl-config.cmake && \
    echo 'set(WOLFSSL_INCLUDE_DIRS "/usr/local/include")'                 >> /usr/local/lib/cmake/wolfssl/wolfssl-config.cmake && \
    echo 'set(WOLFSSL_LIBRARIES "/usr/local/lib/libwolfssl.so")'          >> /usr/local/lib/cmake/wolfssl/wolfssl-config.cmake && \
    echo 'if(NOT TARGET wolfssl::wolfssl)'                                >> /usr/local/lib/cmake/wolfssl/wolfssl-config.cmake && \
    echo '  add_library(wolfssl::wolfssl SHARED IMPORTED)'                >> /usr/local/lib/cmake/wolfssl/wolfssl-config.cmake && \
    echo '  set_target_properties(wolfssl::wolfssl PROPERTIES'            >> /usr/local/lib/cmake/wolfssl/wolfssl-config.cmake && \
    echo '    IMPORTED_LOCATION "/usr/local/lib/libwolfssl.so"'           >> /usr/local/lib/cmake/wolfssl/wolfssl-config.cmake && \
    echo '    INTERFACE_INCLUDE_DIRECTORIES "/usr/local/include")'        >> /usr/local/lib/cmake/wolfssl/wolfssl-config.cmake && \
    echo 'endif()'                                                        >> /usr/local/lib/cmake/wolfssl/wolfssl-config.cmake

# Build libcoap and proxy
# Fix: coap2HttpServer.c uses assert() without including assert.h
RUN sed -i '1s/^/#include <assert.h>\n/' ${PROXYDIR}/coap2http-proxy/src/coap2HttpServer.c && \
    WOLFSSL_GROUPS="${WOLFSSL_GROUPS}" \
    ${PROXYDIR}/coap2http-proxy/install_libcoap.sh && \
    cd ${PROXYDIR}/coap2http-proxy && ./install_proxy.sh

WORKDIR ${PROXYDIR}/coap2http-proxy