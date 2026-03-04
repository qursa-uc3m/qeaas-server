FROM ubuntu:latest

# api path
ARG APIDIR="/home/eaas-quantis-qrng-api"

# api run
ARG APIRUN=${APIDIR}/build

ENV DEBIAN_FRONTEND=noninteractive

# get all software packages required
RUN apt update && apt upgrade -y && \
    apt install -y git gcc g++ cmake libjsoncpp-dev uuid-dev zlib1g-dev openssl libssl-dev

WORKDIR ${APIDIR}

COPY . ${APIDIR}
RUN git clone https://github.com/drogonframework/drogon && cd drogon \
    && git submodule update --init && mkdir build && cd build && cmake .. \
    && make && make install

WORKDIR ${APIDIR}

RUN mkdir build && cd build && cmake .. && make

WORKDIR ${APIRUN}