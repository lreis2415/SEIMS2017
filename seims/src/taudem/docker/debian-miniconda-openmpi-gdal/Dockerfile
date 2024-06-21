##
#
#
# Usage:
#   > cd TauDEM_ext
#   > docker build -t <tag> -f docker/alpine-openmpi-gdal-miniconda/Dockerfile .
#
# Copyright 2024 Liang-Jun Zhu <zlj@lreis.ac.cn>
#

# Use continuumio/miniconda3 as the build and test container, which is based on debian:12.5-slim
# https://github.com/ContinuumIO/docker-images/blob/main/miniconda3/debian/Dockerfile
FROM continuumio/miniconda3 as builder

LABEL maintainer="Liang-Jun Zhu <zlj@lreis.ac.cn>"

RUN apt-get update -q && \
    apt-get install -q -y --no-install-recommends \
    cmake make g++ gdal-bin openmpi-bin libgdal-dev libopenmpi-dev \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# Copy source directory
WORKDIR /taudem_ext
COPY CMakeLists.txt .
COPY cmake cmake
COPY src src
COPY src_ext src_ext
COPY data data

# # Build for release
ARG INSTALL_DIR=/taudem_ext/dist
RUN cd /taudem_ext \
    && mkdir build \
    && cd build \
    && cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR} \
    && make -j8 \
    && make install -j8 \
    && cd ..

# # Build final image
FROM continuumio/miniconda3 as runner

RUN apt-get update -q && \
    apt-get install -q -y --no-install-recommends \
    gdal-bin openmpi-bin libgomp1 tini \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# Order layers starting with less frequently varying ones
ARG INSTALL_DIR=/taudem_ext/dist
COPY --from=builder ${INSTALL_DIR}/bin/ /usr/local/bin/

ENV OMPI_ALLOW_RUN_AS_ROOT=1
ENV OMPI_ALLOW_RUN_AS_ROOT_CONFIRM=1

COPY docker/docker-entrypoint.sh /
ENTRYPOINT ["/usr/bin/tini", "--", "/docker-entrypoint.sh"]
