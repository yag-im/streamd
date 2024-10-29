#ifndef NAVI_H
#define NAVI_H

#include <gst/gst.h>
#include "keyboard.h"
#include "pointer.h"

typedef GstPadProbeReturn (*navi_event_probe_cb_ptr)(GstPad *, GstPadProbeInfo *, gpointer);

struct NaviCapture {
    struct wl_display *display;
    struct wl_seat *seat;
    struct zwlr_virtual_pointer_manager_v1 *pointer_manager;
    struct pointer pointer;
    struct zwp_virtual_keyboard_manager_v1 *keyboard_manager;    
    struct keyboard keyboard;
    navi_event_probe_cb_ptr event_probe_cb;
};

gboolean init_navi_capture(struct NaviCapture **capture);

#endif  // NAVI_H
