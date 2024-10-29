#!/usr/bin/env bash

mkdir -p /workspaces/streamd/.vscode
cp /workspaces/streamd/.devcontainer/settings.json /workspaces/streamd/.vscode

export HOME=/home/${USER}

CHECK_PERIOD=0.05

sudo chown -R ${USER} /dev/dri/card0
sudo chown -R ${USER} /dev/dri/renderD128

dbus-daemon --nofork --print-pid --config-file=/usr/share/dbus-1/session.conf --address=${DBUS_SESSION_BUS_ADDRESS} &

labwc${LABWC_DEBUG} &

while [ ! -f ${XDG_RUNTIME_DIR}/wayland-0.lock ]
do
  sleep ${CHECK_PERIOD}
done

wlr-randr --output ${WLR_OUTPUT} --custom-mode ${SCREEN_WIDTH}x${SCREEN_HEIGHT} &

pipewire &

pipewire-pulse &

wireplumber &

while [ ! -f ${XDG_RUNTIME_DIR}/pipewire-0.lock ]
do
  sleep ${CHECK_PERIOD}
done

/usr/libexec/xdg-desktop-portal-wlr --replace --loglevel ${XDPW_LOG_LEVEL} &

export MOZ_ENABLE_WAYLAND=1 
firefox-esr &
