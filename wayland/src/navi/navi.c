#include "navi.h"

#include </usr/include/linux/input-event-codes.h>
#include <gst/video/navigation.h>
#include <sys/time.h>
#include <unistd.h>
#include <util/bmem.h>

#include "../log.h"

#include "keyboard.h"
#include "pointer.h"
#include "virtual-keyboard-unstable-v1-client-protocol.h"
#include "wlr-virtual-pointer-unstable-v1-client-protocol.h"

#define MOUSE_SCROLL_UP_BUTTON 4
#define MOUSE_SCROLL_DOWN_BUTTON 5

#define CHECK_LAST_ACTIVITY_PERIOD 60
#define DEFAULT_MAX_INACTIVITY_PERIOD "3600"

extern const char keymap_ascii_raw[];

time_t last_activity_time;

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
        if (d > (max_inactivity_period + 2*CHECK_LAST_ACTIVITY_PERIOD)) {
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

static GstPadProbeReturn wl_event_probe_cb(GstPad *pad, GstPadProbeInfo *info, gpointer udata) {
    last_activity_time = time(NULL);

    struct NaviCapture *capture = udata;

    GstPadProbeReturn ret = GST_PAD_PROBE_OK;
    GstEvent *event = GST_PAD_PROBE_INFO_DATA(info);

    gboolean is_press = WL_POINTER_BUTTON_STATE_RELEASED;
    const gchar *keysym_name;  // one of X11 keysym names defined in https://www.cl.cam.ac.uk/~mgk25/ucs/keysyms.txt
    gint button;
    gdouble x, y, delta_x, delta_y;

    switch (gst_navigation_event_get_type(event)) {
        case GST_NAVIGATION_EVENT_KEY_PRESS:
            is_press = WL_POINTER_BUTTON_STATE_PRESSED; /* FALLTHROUGH */
        case GST_NAVIGATION_EVENT_KEY_RELEASE:
            if (gst_navigation_event_parse_key_event(event, &keysym_name)) {
                xkb_keysym_t keysym = xkb_keysym_from_name(keysym_name, XKB_KEYSYM_NO_FLAGS);
                keyboard_feed(&capture->keyboard, keysym, is_press);
            }
            break;
        case GST_NAVIGATION_EVENT_MOUSE_BUTTON_PRESS:
            is_press = WL_POINTER_BUTTON_STATE_PRESSED; /* FALLTHROUGH */
        case GST_NAVIGATION_EVENT_MOUSE_BUTTON_RELEASE:
            if (gst_navigation_event_parse_mouse_button_event(event, &button, &x, &y)) {
                enum nvnc_button_mask mask = 0;
                if (button == 1) {
                    mask |= NVNC_BUTTON_LEFT;
                } else if (button == 2) {
                    mask |= NVNC_BUTTON_MIDDLE;
                } else if (button == 3) {
                    mask |= NVNC_BUTTON_RIGHT;
                }
                if (is_press) {
                    capture->pointer.current_mask = 0;
                } else {
                    capture->pointer.current_mask = mask;
                    mask = 0;
                }
                pointer_set(&capture->pointer, x, y, mask);
            }
            break;
        case GST_NAVIGATION_EVENT_MOUSE_MOVE:
            if (gst_navigation_event_parse_mouse_move_event(event, &x, &y)) {
                pointer_set(&capture->pointer, x, y, capture->pointer.current_mask);
            }
            break;
        case GST_NAVIGATION_EVENT_MOUSE_SCROLL:
            if (gst_navigation_event_parse_mouse_scroll_event(event, &x, &y, &delta_x, &delta_y)) {
                enum nvnc_button_mask mask = 0;
                if ((int)delta_y != 0) {
                    if ((int)delta_y < 0) {
                        mask |= NVNC_SCROLL_DOWN;
                    } else {
                        mask |= NVNC_SCROLL_UP;
                    }
                }
                if ((int)delta_x != 0) {
                    if ((int)delta_x < 0) {
                        mask |= NVNC_SCROLL_LEFT;
                    } else {
                        mask |= NVNC_SCROLL_RIGHT;
                    }
                }
                capture->pointer.current_mask = mask;
                mask = 0;
                pointer_set(&capture->pointer, x, y, mask);
            }
            break;
        default:
            break;
    }
    wl_display_flush(capture->display);
    return ret;
}

static void handle_global(void *data, struct wl_registry *registry, uint32_t name, const char *interface,
                          uint32_t version) {
    if (strcmp(interface, wl_seat_interface.name) == 0) {
        ((struct NaviCapture *)data)->seat = wl_registry_bind(registry, name, &wl_seat_interface, version);
    } else if (strcmp(interface, zwlr_virtual_pointer_manager_v1_interface.name) == 0) {
        ((struct NaviCapture *)data)->pointer_manager =
            wl_registry_bind(registry, name, &zwlr_virtual_pointer_manager_v1_interface, 1);
    } else if (strcmp(interface, zwp_virtual_keyboard_manager_v1_interface.name) == 0) {
        ((struct NaviCapture *)data)->keyboard_manager =
            wl_registry_bind(registry, name, &zwp_virtual_keyboard_manager_v1_interface, 1);
    }
}

static void handle_global_remove(void *data, struct wl_registry *registry, uint32_t name) {
    // Who cares?
}

static const struct wl_registry_listener registry_listener = {
    .global = handle_global,
    .global_remove = handle_global_remove,
};

gboolean init_navi_capture(struct NaviCapture **capture) {
    struct NaviCapture *n_capture = bzalloc(sizeof(struct NaviCapture));
    n_capture->pointer_manager = NULL;
    n_capture->event_probe_cb = wl_event_probe_cb;
    n_capture->seat = NULL;
    n_capture->display = wl_display_connect(getenv("WAYLAND_DISPLAY"));
    if (!n_capture->display) {
        log_error("wl_display_connect error");
        return false;
    }
    struct wl_registry *registry = wl_display_get_registry(n_capture->display);
    wl_registry_add_listener(registry, &registry_listener, n_capture);
    wl_display_roundtrip(n_capture->display);
    if (n_capture->pointer_manager == NULL || n_capture->keyboard_manager == NULL || n_capture->seat == NULL) {
        log_error("compositor does not support wlr-virtual-pointer-unstable-v1");
        return false;
    }

    n_capture->pointer.virtual_pointer =
        zwlr_virtual_pointer_manager_v1_create_virtual_pointer(n_capture->pointer_manager, n_capture->seat);
    n_capture->pointer.screen_height = atoi(getenv("SCREEN_HEIGHT"));
    n_capture->pointer.screen_width = atoi(getenv("SCREEN_WIDTH"));
    if (pointer_init(&n_capture->pointer) < 0) {
        log_error("pointer_init error");
        return false;
    }

    n_capture->keyboard.virtual_keyboard =
        zwp_virtual_keyboard_manager_v1_create_virtual_keyboard(n_capture->keyboard_manager, n_capture->seat);
    struct xkb_rule_names rule_names = {
        .rules = "base",
        .layout = "us",
        .model = "pc105",
        .variant = "",
        .options = "",
    };
    if (keyboard_init(&n_capture->keyboard, &rule_names) < 0) {
        log_error("keyboard_init error");
        return false;
    }
    *capture = n_capture;

    pthread_t thread_id;
    if (pthread_create(&thread_id, NULL, &check_last_activity, NULL) != 0) {
        log_error("failed to create watchdog thread");
        return false;
    }

    return true;
}
