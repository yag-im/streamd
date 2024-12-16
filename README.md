# streamd

`streamd` is a WebRTC streaming daemon that runs within each Jukebox container, supporting both X11 (ximagesrc) and 
Wayland (pipewiresrc) for frame capture. It offers frame encoding using CPU, iGPU (Intel HD), and GPU (Nvidia). 
The `streamd` component is baked into each [Jukebox](https://github.com/yag-im/jukebox) image.

## Build notes

Generate header file

    wayland-scanner client-header < /workspaces/jukebox/runners/base/deps/wlroots/protocol/virtual-keyboard-unstable-v1.xml > virtual-keyboard-unstable-v1-client-protocol.h
    wayland-scanner client-header < /workspaces/jukebox/runners/base/deps/wlroots/protocol/wlr-virtual-pointer-unstable-v1.xml > wlr-virtual-pointer-unstable-v1-client-protocol.h

Generate source file

    wayland-scanner private-code < /workspaces/jukebox/runners/base/deps/wlroots/protocol/virtual-keyboard-unstable-v1.xml > virtual-keyboard-unstable-v1-client-protocol.c
    wayland-scanner private-code < /workspaces/jukebox/runners/base/deps/wlroots/protocol/wlr-virtual-pointer-unstable-v1.xml > wlr-virtual-pointer-unstable-v1-client-protocol.c

Env vars used at runtime

    DISPLAY (for X11)
    SHOW_POINTER (for X11)
    FPS
    MAX_INACTIVITY_PERIOD
    SCREEN_WIDTH
    SCREEN_HEIGHT
    SIGNALER_URI
    SIGNALER_HOST (must be sent as a header for valid routing; can't be extracted from SIGNALER_URI as it may use IP address (e.g. in local setups only))
    SIGNALER_AUTH_TOKEN (secret)
    STUN_URI
    WAYLAND_DISPLAY (for Wayland)
    WS_CONN_ID
    WS_CONSUMER_ID    

## Refs

### Wayland navigation

pipewire, screencast-portal: https://github.com/obsproject/obs-studio/tree/master/plugins/linux-pipewire

pointer: https://github.com/swaywm/wlroots/blob/master/examples/virtual-pointer.c

keyboard: https://github.com/any1/wayvnc
