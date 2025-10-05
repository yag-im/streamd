#include "utils.h"

#include <stdio.h>

#include "log.h"

char *getenv_or_default(char *name, char *dflt) {
    char *val = getenv(name);
    if (val) {
        return val;
    }
    return dflt;
}

time_t last_activity_time;

void *check_last_activity() {
    last_activity_time = time(NULL);
    float max_inactivity_duration = atof(getenv_or_default("MAX_INACTIVITY_DURATION", DEFAULT_MAX_INACTIVITY_DURATION));
    log_info("max inactivity period: %f seconds", max_inactivity_duration);
    while (1) {
        sleep(CHECK_LAST_ACTIVITY_INTERVAL);
        time_t current_time = time(NULL);
        float d = difftime(current_time, last_activity_time);
        log_info("checking for inactivity, last activity: %f seconds ago", d);
        if (d > (max_inactivity_duration + 2 * CHECK_LAST_ACTIVITY_INTERVAL)) {
            // probably container was resumed after a pause, resetting last_activity_time
            last_activity_time = time(NULL);
            d = 0;
        }
        if (d >= max_inactivity_duration) {
            log_info("exiting due to inactivity");
            exit(0);
        }
    }
    return NULL;
}

void *countdown_overlay_thread(void *data) {
    CountdownArgs *args = (CountdownArgs *)data;
    GstElement *overlay = args->overlay;
    int seconds = args->seconds;
    char text[128];
    for (int i = seconds; i > 0; --i) {
        snprintf(text, sizeof(text), "Please wait, loading... %d", i);
        g_object_set(overlay, "text", text, NULL);
        sleep(1);
    }
    g_object_set(overlay, "text", "", NULL);
    return NULL;
}