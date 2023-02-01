FROM fedora:37

ARG APP_UID=2000

ARG APP_GID=2000

ARG JPEG_LIB=libjpeg-turbo-devel

MAINTAINER TANGO Controls team <contact@tango-controls.org>

RUN dnf -y update && dnf clean all

RUN dnf -y install \
  make automake gcc gcc-c++ \
  pkgconf-pkg-config        \
  cmake                     \
  curl                      \
  git                       \
  openssl-devel             \
  python-devel              \
  zeromq-devel              \
  ${JPEG_LIB}               \
  python3

RUN groupadd -g "$APP_GID" tango

RUN useradd -u "$APP_UID" -g "$APP_GID" -ms /bin/bash tango

USER tango

WORKDIR /home/tango
