#ifndef NAVI_H
#define NAVI_H

#include <X11/extensions/XTest.h>

#include <gst/gst.h>

    typedef GstPadProbeReturn (*navi_event_probe_cb_ptr)(GstPad *, GstPadProbeInfo *, gpointer);

struct NaviCapture {
    Display *display;
    navi_event_probe_cb_ptr event_probe_cb;
};

void gst_ximage_navigation_mouse_move_pointer(Display *display, int x, int y, Bool is_relative);
void gst_ximage_navigation_mouse_push_button(Display *display, unsigned int button, Bool is_press);
void gst_ximage_navigation_key(Display *display, const char *keysym_name, Bool is_press);
gboolean init_navi_capture(struct NaviCapture **capture);

#endif  // NAVI_H
