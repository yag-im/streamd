ARG BUILDER_BASE_IMAGE="debian:bookworm-slim"
ARG RUNNER_BASE_IMAGE="debian:bookworm-slim"

ARG BUILD_DIR=/tmp/build

FROM ${BUILDER_BASE_IMAGE} as builder

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

# dep: wayland (required for intel-wayland deps)
ARG WAYLAND_VERSION=1.22.0
RUN apt-get install -y --no-install-recommends \
    libexpat1-dev \
    libffi-dev \
    libxml2-dev
RUN git clone --depth=1 --single-branch --branch ${WAYLAND_VERSION} https://gitlab.freedesktop.org/wayland/wayland.git
# COPY deps/wayland wayland
RUN cd wayland \
    && meson setup \
        --prefix=$BUILD_DIR \
        -Ddocumentation=false \
        -Dtests=false \
        build \
    && ninja -C build install

# dep: intel-libva
ARG LIBVA_VERSION=2.20.0
ARG LIBVA_URL=https://github.com/intel/libva/releases/download/${LIBVA_VERSION}/libva-${LIBVA_VERSION}.tar.bz2
RUN apt-get install -y --no-install-recommends \
    curl \
    libdrm-dev \
    libx11-dev \
    libxext-dev \
    libxfixes-dev \
    libxcb-dri3-dev \
    libx11-xcb-dev
RUN mkdir libva \
    && curl -# -L -f ${LIBVA_URL} | tar xj --strip 1 -C libva \
    && cd libva \
    && ./configure \
        --prefix=$BUILD_DIR \
        --enable-x11 \
        --disable-glx \
        --enable-wayland \
        --disable-static \
        --enable-shared \
        --libdir=$BUILD_DIR/lib/x86_64-linux-gnu \
    && make -C $SRC_DIR/libva -j${BUILD_NUM_WORKERS} \
    && make -C $SRC_DIR/libva install

# dep: intel-vaapi-driver
ARG INTEL_VAAPI_DRIVER_VERSION=2.4.1
ARG INTEL_VAAPI_DRIVER_URL=https://github.com/intel/intel-vaapi-driver/releases/download/${INTEL_VAAPI_DRIVER_VERSION}/intel-vaapi-driver-${INTEL_VAAPI_DRIVER_VERSION}.tar.bz2
RUN mkdir intel-vaapi-driver \
    && curl -# -L -f ${INTEL_VAAPI_DRIVER_URL} | tar xj --strip 1 -C intel-vaapi-driver \
    && cd intel-vaapi-driver \
    && ./configure \
        --prefix=$BUILD_DIR \
    && make -C $SRC_DIR/intel-vaapi-driver -j${BUILD_NUM_WORKERS} \
    && make -C $SRC_DIR/intel-vaapi-driver install

# dep: intel-media-driver
ARG GMMLIB_VERSION=22.3.14
ARG GMMLIB_URL=https://github.com/intel/gmmlib/archive/intel-gmmlib-${GMMLIB_VERSION}.tar.gz
ARG INTEL_MEDIA_DRIVER_VERSION=23.3.5
ARG INTEL_MEDIA_DRIVER_URL=https://github.com/intel/media-driver/archive/intel-media-${INTEL_MEDIA_DRIVER_VERSION}.tar.gz
# intel-media-driver looks in ../gmmlib
RUN mkdir gmmlib \
     && curl -# -L -f ${GMMLIB_URL} | tar xz --strip 1 -C gmmlib
RUN mkdir intel-media-driver \
    && curl -# -L -f ${INTEL_MEDIA_DRIVER_URL} | tar xz --strip 1 -C intel-media-driver \
    && mkdir intel-media-driver/build \
    && cd intel-media-driver/build \
    && cmake \
        -DCMAKE_FIND_ROOT_PATH=/ \
        -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY \
        -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY \
        -DCMAKE_FIND_ROOT_PATH_MODE_PACKAGE=ONLY \
        -DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=NEVER \
        -DCMAKE_INSTALL_PREFIX=$BUILD_DIR/lib/x86_64-linux-gnu \
        -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
        -Wno-dev \
        -DBUILD_TYPE=$BUILD_TYPE \
        -DINSTALL_DRIVER_SYSCONF=OFF \
        -DMEDIA_RUN_TEST_SUITE=OFF \
        -DSKIP_GMM_CHECK=ON \
        ../ \
    && make -C $SRC_DIR/intel-media-driver/build -j${BUILD_NUM_WORKERS} \
    && make -C $SRC_DIR/intel-media-driver/build install

# cmake puts all artifacts (dri/... and lib/...) under DCMAKE_INSTALL_PREFIX, we need to move lib/... to the proper place
RUN cp -r $BUILD_DIR/lib/x86_64-linux-gnu/lib/* $BUILD_DIR/lib/x86_64-linux-gnu && rm -rf $BUILD_DIR/lib/x86_64-linux-gnu/lib

# dep: oneVPL-intel-gpu
ARG INTEL_ONEVPL_GPU_RUNTIME_VERSION=23.3.4
ARG INTEL_ONEVPL_GPU_RUNTIME_URL=https://github.com/oneapi-src/oneVPL-intel-gpu/archive/refs/tags/intel-onevpl-${INTEL_ONEVPL_GPU_RUNTIME_VERSION}.tar.gz
RUN mkdir oneVPL-intel-gpu \
    && curl -# -L -f ${INTEL_ONEVPL_GPU_RUNTIME_URL} | tar xz --strip 1 -C oneVPL-intel-gpu \
    && mkdir oneVPL-intel-gpu/build \
    && cd oneVPL-intel-gpu/build \
    && cmake \
        -DCMAKE_FIND_ROOT_PATH=/ \
        -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY \
        -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY \
        -DCMAKE_FIND_ROOT_PATH_MODE_PACKAGE=ONLY \
        -DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=NEVER \
        -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
        -DCMAKE_INSTALL_PREFIX=$BUILD_DIR \
        ../ \
    && make -C $SRC_DIR/oneVPL-intel-gpu/build -j${BUILD_NUM_WORKERS} \
    && make -C $SRC_DIR/oneVPL-intel-gpu/build install

# dep: wayland-protocols
ARG WAYLAND_PROTOCOLS_VERSION=1.32
RUN git clone --depth=1 --single-branch --branch ${WAYLAND_PROTOCOLS_VERSION} https://gitlab.freedesktop.org/wayland/wayland-protocols.git \
    && cd wayland-protocols \
    && meson setup \
        --prefix=$BUILD_DIR \
        -Dtests=false \
        build \
    && ninja -C build install

# dep: mesa-drm
ARG LIBDRM_VERSION=2.4.118
RUN git clone --depth=1 --single-branch --branch libdrm-${LIBDRM_VERSION} https://gitlab.freedesktop.org/mesa/drm.git \
    && cd drm \
    && meson setup \
        --prefix=$BUILD_DIR \
        -Dtests=false \
        -Dvalgrind=disabled \
        -Dman-pages=disabled \
        build \
    && ninja -C build install

# dep: libinput
ARG LIBINPUT_VERSION=1.24.0
RUN apt-get install -y --no-install-recommends \
    libevdev-dev \
    libgtk-4-dev \
    libmtdev-dev \    
    libudev-dev
RUN git clone --depth=1 --single-branch --branch ${LIBINPUT_VERSION} https://gitlab.freedesktop.org/libinput/libinput.git \
    && cd libinput \
    && meson setup \
        --prefix=$BUILD_DIR \
        -Dlibwacom=false \
        -Dtests=false \
        build \
    && ninja -C build install

# dep: libdisplay-info
ARG LIBDISPLAY_VERSION=0.1.1
RUN apt-get install -y \
    hwdata
RUN git clone --depth=1 --single-branch --branch ${LIBDISPLAY_VERSION} https://gitlab.freedesktop.org/emersion/libdisplay-info \
    && cd libdisplay-info \
    && meson setup \
        --prefix=$BUILD_DIR \
    build \
    && ninja -C build install

# dep: wlroots
ARG WLROOTS_VERSION=0.16.2
RUN apt-get install -y \
    hwdata \
    libseat-dev \
    libgbm-dev
# COPY deps/wlroots wlroots
RUN git clone --depth=1 --single-branch --branch ${WLROOTS_VERSION} https://gitlab.freedesktop.org/wlroots/wlroots.git
RUN cd wlroots \
    && meson setup \
        --prefix=$BUILD_DIR \
        -Dxwayland=disabled \
        -Dexamples=false \
        -Dbackends=drm,libinput \
        -Drenderers=gles2 \
        -Dallocators=gbm \
    build \
    && ninja -C build install

# dep: basu
ARG BASU_VERSION=0.2.1
RUN apt-get install -y --no-install-recommends \
    gperf
RUN git clone --depth=1 --single-branch --branch v${BASU_VERSION} https://git.sr.ht/~emersion/basu \
    && cd basu \
    && meson setup \
        --prefix=$BUILD_DIR \
        build \
    && ninja -C build install

# dep: xdg-desktop-portal-wlr
ARG XDPW_VERSION=0.7.0
RUN apt-get install -y --no-install-recommends \
    libgbm-dev \
    libinih-dev \
    libpipewire-0.3-dev
RUN git clone --depth=1 --single-branch --branch v${XDPW_VERSION} https://github.com/emersion/xdg-desktop-portal-wlr \
    && cd xdg-desktop-portal-wlr \
    && meson setup \
        --prefix=$BUILD_DIR \
        -Dsd-bus-provider=basu \
        -Dsystemd=disabled \
        -Dman-pages=disabled \
        build \
    && ninja -C build install

# dep: labwc
ARG LABWC_VERSION=0.6.5
RUN apt-get install -y --no-install-recommends \
    hwdata \
    libcairo-dev \
    libglib2.0-dev \
    libpango1.0-dev \
    libpixman-1-dev \
    libseat-dev \
    libudev-dev \
    libxkbcommon-dev
RUN git clone --depth=1 --single-branch --branch ${LABWC_VERSION} https://github.com/labwc/labwc.git \
    && cd labwc \
    && meson setup \
        --wrap-mode=nodownload \
        --prefix=$BUILD_DIR \
        -Dman-pages=disabled \
        -Dxwayland=disabled \
        -Dsvg=disabled \
        -Dnls=disabled \
        build \
    && ninja -C build install

# dep: cage
# ARG CAGE_VERSION=0.1.5
# RUN git clone --depth=1 --single-branch --branch v${CAGE_VERSION} https://github.com/cage-kiosk/cage \
#     && cd cage \
#     && meson setup \
#         --wrap-mode=nodownload \
#         --prefix=$BUILD_DIR \
#         -Dman-pages=disabled \
#         -Dxwayland=false \
#         build \
#     && ninja -C build install

# dep: pipewire
ARG PIPEWIRE_VERSION=1.0.4
RUN apt-get install -y --no-install-recommends \
    libdbus-glib-1-dev \
    libgstreamer1.0-dev \
    libgstreamer-plugins-base1.0-dev \
    libpulse-dev
RUN git clone --depth=1 --single-branch --branch ${PIPEWIRE_VERSION} https://github.com/PipeWire/pipewire.git
# COPY deps/pipewire pipewire
RUN cd pipewire \
    && meson setup \
        --prefix=$BUILD_DIR \
        -Dexamples=disabled \
        -Dman=disabled \
        -Dtests=disabled \
        -Dgstreamer=enabled \
        -Dsystemd=disabled \
        -Dselinux=disabled \
        -Dpipewire-alsa=disabled \
        -Dpipewire-jack=disabled \
        -Dpipewire-v4l2=disabled \
        -Dalsa=disabled \
        -Djack=disabled \
        -Dlibcamera=disabled \
        -Dlibpulse=enabled \
        -Dlibusb=disabled \
        -Dsession-managers="[ 'wireplumber' ]" \
        -Dx11=disabled \
        build \
    && ninja -C build install

# runner
FROM ${RUNNER_BASE_IMAGE} as runner

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

COPY --from=builder $BUILD_DIR /usr

RUN mkdir -p /usr/share/xdg-desktop-portal-wlr
COPY wayland/files/xdg-desktop-portal-wlr.conf /usr/share/xdg-desktop-portal-wlr/config

RUN chown -R $USERNAME:$USERNAME /home/$USERNAME

COPY deps/gstreamer/subprojects/packages/bin /bin
COPY deps/gstreamer/subprojects/packages/include /include
COPY deps/gstreamer/subprojects/packages/lib /lib
COPY deps/gstreamer/subprojects/packages/share /share
COPY deps/gstreamer/subprojects/packages/libexec /usr/libexec

RUN ln -s /usr/libexec /libexec
RUN ldconfig
