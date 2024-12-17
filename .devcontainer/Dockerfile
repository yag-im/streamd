ARG BASE_IMAGE="debian:bookworm"

ARG BUILD_DIR=/tmp/build

FROM ${BASE_IMAGE} as builder

ARG BUILD_DIR
ARG BUILD_NUM_WORKERS=8
ARG BUILD_TYPE=Release
ARG SRC_DIR=/tmp/src

ENV DEBIAN_FRONTEND noninteractive
ENV PKG_CONFIG_PATH $BUILD_DIR/lib/x86_64-linux-gnu/pkgconfig/:$BUILD_DIR/share/pkgconfig/

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        automake \
        build-essential \
        ca-certificates \
        cmake \
        git \
        ninja-build \
        pip \
        pkg-config

RUN pip install meson --break-system-packages

WORKDIR $SRC_DIR

COPY artifacts/build/ .
RUN ./build_gstreamer.sh $BUILD_DIR

# runner
FROM ${BASE_IMAGE} as runner

# TODO: use $USER build arg for USERNAME
ARG BUILD_DIR
ARG USERNAME=gamer
ARG USER_UID=1000
ARG USER_GID=$USER_UID

ENV DEBIAN_FRONTEND noninteractive

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        build-essential \
        clang-format \
        cmake \
        dbus-daemon \
        firefox-esr intel-media-va-driver- \
        gdb \
        git \
        hwdata \
        less \
        libcairo-dev \
        libdbus-glib-1-dev \
        libevdev-dev \
        libgbm-dev \
        libgirepository1.0-dev \
        libgles-dev \
        libglib2.0-dev \
        libinih-dev \
        libmtdev-dev \
        libpango1.0-dev \
        libpipewire-0.3-dev \
        libpixman-1-dev \
        libpulse-dev \
        libseat-dev \
        libsrtp2-dev \
        libudev-dev \
        libxtst-dev \
        libxkbcommon-dev \
        openssh-client \
        openssl \
        procps \
        rtkit \
        sudo \
        vim \
        wlr-randr \
        xdg-desktop-portal \
    && apt-get clean -y \
    && rm -rf /var/lib/apt/lists/* /var/cache/apt/*

COPY --from=builder $BUILD_DIR /usr

# create non-root user TODO: drop sudo
# TODO: sync /run/user/$USER_UID with XDG_RUNTIME_DIR
RUN groupadd --gid $USER_GID $USERNAME \
    && groupadd render \
    && useradd --uid $USER_UID --gid $USER_GID --shell /bin/bash --create-home $USERNAME \
    && usermod -aG sudo,audio,video,rtkit,render $USERNAME \
    && mkdir -p /etc/$USERNAME /var/log/$USERNAME /run/user/$USER_UID \
    && chmod 1777 /var/log/$USERNAME \
    && chown $USERNAME /var/log/$USERNAME \
    && chmod 0700 /run/user/$USER_UID \
    && chown $USERNAME /run/user/$USER_UID \
    && echo $USERNAME ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/$USERNAME \
    && chmod 0440 /etc/sudoers.d/$USERNAME

RUN chown -R $USERNAME:$USERNAME /home/$USERNAME

RUN mkdir -p /usr/share/xdg-desktop-portal-wlr
# keep in sync with jukebox/runners/base/wayland/files/xdg-desktop-portal-wlr.conf
COPY artifacts/runtime/conf/xdg-desktop-portal-wlr.conf /usr/share/xdg-desktop-portal-wlr/config
