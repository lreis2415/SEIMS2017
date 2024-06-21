##
# crazyzlj/ccgl:alpine-with-gdal-mongodb
#
# This script is written based on PROJ and GDAL.
#   https://github.com/OSGeo/PROJ/blob/master/Dockerfile
#   https://github.com/OSGeo/gdal/tree/master/docker
#
# Usage:
#   > cd CCGL
#   > docker build -t <tag> -f docker/apline-with-gdal-mongodb/Dockerfile .
#
# Copyright 2022-2024 Liang-Jun Zhu <zlj@lreis.ac.cn>

# Use GDAL image tagged alpine-small-latest as the build and test container
FROM ghcr.io/osgeo/gdal:alpine-small-latest as builder

LABEL maintainer="Liang-Jun Zhu <zlj@lreis.ac.cn>"

# RUN sed -i 's/dl-cdn.alpinelinux.org/mirrors.tuna.tsinghua.edu.cn/g' /etc/apk/repositories && \
# RUN sed -i 's/dl-cdn.alpinelinux.org/mirrors.aliyun.com/g' /etc/apk/repositories && \
RUN apk update && apk upgrade && \
    apk add --no-cache cmake make g++ mongo-c-driver-dev

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
FROM ghcr.io/osgeo/gdal:alpine-small-latest as runner

# Replace alpine repository source cdn
# Add gnu standard C++ library and OpenMP library for running apps based on CCGL
# RUN sed -i 's/dl-cdn.alpinelinux.org/mirrors.tuna.tsinghua.edu.cn/g' /etc/apk/repositories && \
# RUN sed -i 's/dl-cdn.alpinelinux.org/mirrors.aliyun.com/g' /etc/apk/repositories && \
RUN apk update && apk upgrade && \
    apk add --no-cache libstdc++ libgomp mongo-c-driver

# Order layers starting with less frequently varying ones
ARG INSTALL_DIR=/CCGL/dist
COPY --from=builder ${INSTALL_DIR}/bin/ /usr/local/bin/
COPY --from=builder ${INSTALL_DIR}/include/ /usr/local/include/
COPY --from=builder ${INSTALL_DIR}/lib/ /usr/local/lib/
COPY --from=builder ${INSTALL_DIR}/share/ccgl/ /usr/local/share/ccgl/
