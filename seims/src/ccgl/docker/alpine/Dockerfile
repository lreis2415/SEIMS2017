##
# crazyzlj/ccgl:alpine
#
# This script is written based on PROJ and GDAL.
#   https://github.com/OSGeo/PROJ/blob/master/Dockerfile
#   https://github.com/OSGeo/gdal/tree/master/docker
#
# Usage:
#   > cd CCGL
#   > docker build -t <tag> -f docker/apline/Dockerfile .
#
# Copyright 2022-2024 Liang-Jun Zhu <zlj@lreis.ac.cn>

# Use alpine as the build container
ARG ALPINE_VERSION=3.15
FROM alpine:${ALPINE_VERSION} as builder

LABEL maintainer="Liang-Jun Zhu <zlj@lreis.ac.cn>"

# Replace alpine repository source cdn to accelarate access speed and setup build env

# RUN sed -i 's/dl-cdn.alpinelinux.org/mirrors.tuna.tsinghua.edu.cn/g' /etc/apk/repositories && \
# RUN sed -i 's/dl-cdn.alpinelinux.org/mirrors.aliyun.com/g' /etc/apk/repositories && \
RUN apk update && apk upgrade && \
    apk add --no-cache cmake make g++

# Copy source directory
WORKDIR /CCGL
COPY CMakeLists.txt .
COPY cmake cmake
COPY src src
COPY apps apps
COPY test test
COPY data data

# # Build for release
ARG INSTALL_DIR=/CCGL/dist
RUN cd /CCGL \
    && mkdir build \
    && cd build \
    && cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR} \
    && make -j 2 \
    && make install \
    && cd ..

# # Build final image
FROM alpine:${ALPINE_VERSION} as runner

# Replace alpine repository source cdn
# Add gnu standard C++ library and OpenMP library for running apps based on CCGL
# RUN sed -i 's/dl-cdn.alpinelinux.org/mirrors.tuna.tsinghua.edu.cn/g' /etc/apk/repositories && \
# RUN sed -i 's/dl-cdn.alpinelinux.org/mirrors.aliyun.com/g' /etc/apk/repositories && \
RUN apk update && apk upgrade && \
    apk add --no-cache libstdc++ libgomp

# Order layers starting with less frequently varying ones
ARG INSTALL_DIR=/CCGL/dist
COPY --from=builder ${INSTALL_DIR}/bin/ /usr/local/bin/
COPY --from=builder ${INSTALL_DIR}/include/ /usr/local/include/
COPY --from=builder ${INSTALL_DIR}/lib/ /usr/local/lib/
COPY --from=builder ${INSTALL_DIR}/share/ccgl/ /usr/local/share/ccgl/
