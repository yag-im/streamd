#include "./navi.h"

#include <gst/video/navigation.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>

#include "./log.h"

#define MOUSE_SCROLL_UP_BUTTON 4
#define MOUSE_SCROLL_DOWN_BUTTON 5

#define CHECK_LAST_ACTIVITY_PERIOD 60
#define DEFAULT_MAX_INACTIVITY_PERIOD "3600"
#define DEFAULT_FORCE_RELATIVE_MOUSE_MOVES "false"

// Based on xtestlib: https://www.x.org/releases/X11R7.5/doc/Xext/xtestlib.html

time_t last_activity_time;
int cur_mouse_pos_x = -1;
int cur_mouse_pos_y = -1;
bool force_relative_mouse_moves = false;

char *getenv_or_default(char *name, char *dflt) {
    char *val = getenv(name);
    if (val) {
        return val;
    }
    return dflt;
}

void *check_last_activity() {
    last_activity_time = time(NULL);
    float max_inactivity_period = atof(getenv_or_default("MAX_INACTIVITY_PERIOD", DEFAULT_MAX_INACTIVITY_PERIOD));
    log_info("max inactivity period: %f seconds", max_inactivity_period);
    while (1) {
        sleep(CHECK_LAST_ACTIVITY_PERIOD);
        time_t current_time = time(NULL);
        float d = difftime(current_time, last_activity_time);
        log_info("checking for inactivity, last activity: %f seconds ago", d);
        if (d > (max_inactivity_period + 2 * CHECK_LAST_ACTIVITY_PERIOD)) {
            // probably container was resumed after a pause, resetting last_activity_time
            last_activity_time = time(NULL);
            d = 0;
        }
        if (d >= max_inactivity_period) {
            log_info("exiting due to inactivity");
            exit(0);
        }
    }
    return NULL;
}

void gst_ximage_navigation_mouse_move_pointer(Display *display, int x, int y, Bool is_relative) {
    if (!is_relative) {
        // If screen_number is -1, the current screen (that the pointer is on) is used
        // log_debug("XTestFakeMotionEvent: x: %d, y: %d", x, y);
        XTestFakeMotionEvent(display, -1, x, y, CurrentTime);
    } else {
        int dx = x;
        int dy = y;
        if (force_relative_mouse_moves && !is_relative) {
            // forced relative mouse moves
            if (cur_mouse_pos_x < 0) cur_mouse_pos_x = x;
            if (cur_mouse_pos_y < 0) cur_mouse_pos_y = y;
            dx = x - cur_mouse_pos_x;
            dy = y - cur_mouse_pos_y;
        }
        // log_debug("XTestFakeRelativeMotionEvent: dx: %d, dy: %d", dx, dy);
        XTestFakeRelativeMotionEvent(display, dx, dy, CurrentTime);
        cur_mouse_pos_x = x;
        cur_mouse_pos_y = y;
    }
    XSync(display, false);
    return;
}

void gst_ximage_navigation_mouse_push_button(Display *display, unsigned int button, Bool is_press) {
    /*
       button values:
       1 = left button
       2 = middle button (pressing the scroll wheel)
       3 = right button
       4 = turn scroll wheel up
       5 = turn scroll wheel down
       6 = push scroll wheel left
       7 = push scroll wheel right
       8 = 4th button (aka browser backward button)
       9 = 5th button (aka browser forward button)
     */
    XTestFakeButtonEvent(display, button, is_press, CurrentTime);
    XSync(display, false);
    return;
}

void gst_ximage_navigation_key(Display *display, const char *keysym_name, Bool is_press) {
    // keysym_name: one of X11 keysym names defined in https://www.cl.cam.ac.uk/~mgk25/ucs/keysyms.txt
    unsigned int keysym, keycode;
    keysym = (unsigned int)XStringToKeysym(keysym_name);
    keycode = XKeysymToKeycode(display, keysym);
    log_debug("keyboard event: keycode=%u state=%s", keycode, is_press ? "pressed" : "released");
    if (keycode == 0)  // undefined KeySym
        return;
    XTestFakeKeyEvent(display, keycode, is_press, CurrentTime);
    XSync(display, false);
    return;
}

static GstPadProbeReturn x11_event_probe_cb(GstPad *pad, GstPadProbeInfo *info, gpointer udata) {
    last_activity_time = time(NULL);

    struct NaviCapture *capture = udata;

    GstPadProbeReturn ret = GST_PAD_PROBE_OK;
    GstEvent *event = GST_PAD_PROBE_INFO_DATA(info);

    gboolean is_press = false;

    const gchar *key;
    gint button;
    gdouble x, y, delta_x, delta_y;
    GstNavigationModifierType modifier_state;
    Bool is_relative = False;

    switch (gst_navigation_event_get_type(event)) {
        case GST_NAVIGATION_EVENT_KEY_PRESS:
            is_press = true; /* FALLTHROUGH */
        case GST_NAVIGATION_EVENT_KEY_RELEASE:
            if (gst_navigation_event_parse_key_event(event, &key)) {
                gst_ximage_navigation_key(capture->display, key, is_press);
                ret = true;
            }
            break;
        case GST_NAVIGATION_EVENT_MOUSE_BUTTON_PRESS:
            is_press = true; /* FALLTHROUGH */
        case GST_NAVIGATION_EVENT_MOUSE_BUTTON_RELEASE:
            if (gst_navigation_event_parse_mouse_button_event(event, &button, &x, &y)) {
                gst_ximage_navigation_mouse_push_button(capture->display, button, is_press);
                ret = true;
            }
            break;
        case GST_NAVIGATION_EVENT_MOUSE_MOVE:
            if (gst_navigation_event_parse_mouse_move_event(event, &x, &y)) {
                if (gst_navigation_event_parse_modifier_state(event, &modifier_state)) {
                    is_relative = modifier_state & GST_NAVIGATION_MODIFIER_LOCK_MASK;
                }
                gst_ximage_navigation_mouse_move_pointer(capture->display, (int)x, (int)y, is_relative);
                ret = true;
            }
            break;
        case GST_NAVIGATION_EVENT_MOUSE_SCROLL:
            if (gst_navigation_event_parse_mouse_scroll_event(event, &x, &y, &delta_x, &delta_y)) {
                int scroll_button = (int)delta_y < 0 ? MOUSE_SCROLL_DOWN_BUTTON : MOUSE_SCROLL_UP_BUTTON;
                gst_ximage_navigation_mouse_push_button(capture->display, scroll_button, true);
                gst_ximage_navigation_mouse_push_button(capture->display, scroll_button, false);
                ret = true;
            }
            break;
        default:
            break;
    }

    return ret;
}

gboolean init_navi_capture(struct NaviCapture **capture) {
    struct NaviCapture *n_capture = malloc(sizeof(struct NaviCapture));
    n_capture->event_probe_cb = x11_event_probe_cb;
    n_capture->display = XOpenDisplay(getenv("DISPLAY"));
    if (n_capture->display == NULL) {
        log_error("init_navi_capture error");
        return false;
    }
    *capture = n_capture;

    pthread_t thread_id;
    if (pthread_create(&thread_id, NULL, &check_last_activity, NULL) != 0) {
        log_error("failed to create watchdog thread");
        return false;
    }

    force_relative_mouse_moves =
        !strcmp(getenv_or_default("FORCE_RELATIVE_MOUSE_MOVES", DEFAULT_FORCE_RELATIVE_MOUSE_MOVES), "true");
    if (force_relative_mouse_moves) {
        log_info("forced relative mouse moves are enabled");
    }

    return true;
}
