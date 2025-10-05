#ifndef UTILS_H
#define UTILS_H

#include <gst/gst.h>
#include <unistd.h>

#define CHECK_LAST_ACTIVITY_INTERVAL 60
#define DEFAULT_MAX_INACTIVITY_DURATION "3600"

extern time_t last_activity_time;

char *getenv_or_default(char *name, char *dflt);

void *check_last_activity();

typedef struct {
    GstElement *overlay;
    int seconds;
} CountdownArgs;
void *countdown_overlay_thread(void *data);

#endif  // UTILS_H
