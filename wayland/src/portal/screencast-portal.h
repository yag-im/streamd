#ifndef SCREENCAST_PORTAL_H
#define SCREENCAST_PORTAL_H

#include <gio/gio.h>

#include "util/c99defs.h"

typedef void (*on_pipewire_stream_ready_cb_ptr)(int);

enum portal_capture_type {
    PORTAL_CAPTURE_TYPE_MONITOR = 1 << 0,
    PORTAL_CAPTURE_TYPE_WINDOW = 1 << 1,
    PORTAL_CAPTURE_TYPE_VIRTUAL = 1 << 2,
};

enum portal_cursor_mode {
    PORTAL_CURSOR_MODE_HIDDEN = 1 << 0,
    PORTAL_CURSOR_MODE_EMBEDDED = 1 << 1,
    PORTAL_CURSOR_MODE_METADATA = 1 << 2,
};

struct ScreencastPortalCapture {
    enum portal_capture_type capture_type;

    GCancellable *cancellable;

    char *session_handle;
    char *restore_token;

    uint32_t pipewire_node;
    bool cursor_visible;

    on_pipewire_stream_ready_cb_ptr stream_ready_cb;
};

gboolean init_screencast_capture(struct ScreencastPortalCapture *capture);

#endif  // SCREENCAST_PORTAL_H
