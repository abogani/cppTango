FROM debian:bullseye

ARG LLVM_VERSION=16

ARG APP_UID=2000

ARG APP_GID=2000

ARG JPEG_LIB=libjpeg-dev

MAINTAINER TANGO Controls team <tango@esrf.fr>

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
  apt-utils                              \
  build-essential                        \
  cmake                                  \
  curl                                   \
  git                                    \
  gnupg                                  \
  libcos4-dev                            \
  libomniorb4-dev                        \
  libomnithread4-dev                     \
  libzmq3-dev                            \
  lsb-release                            \
  omniidl                                \
  ${JPEG_LIB}                            \
  python3

RUN curl https://apt.llvm.org/llvm-snapshot.gpg.key | apt-key add - &&                                                  \
    echo "deb http://apt.llvm.org/bullseye/ llvm-toolchain-bullseye-${LLVM_VERSION} main" | tee -a /etc/apt/sources.list && \
    apt-get update -qq &&                                                                                               \
    apt-get install --no-install-recommends -y                                                                          \
        clang-${LLVM_VERSION}                                                                                           \
        lld-${LLVM_VERSION}                                                                                             \
        llvm-${LLVM_VERSION}                                                                                            \
        libc++-${LLVM_VERSION}-dev                                                                                      \
        libc++abi-${LLVM_VERSION}-dev

RUN groupadd -g "$APP_GID" tango

RUN useradd -u "$APP_UID" -g "$APP_GID" -ms /bin/bash tango

ENV PKG_CONFIG_PATH=/home/tango/lib/pkgconfig
ENV CXX=clang++-${LLVM_VERSION}
ENV CC=clang-${LLVM_VERSION}

USER tango

WORKDIR /home/tango
