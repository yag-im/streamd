#include <gst/gst.h>
#include <stdio.h>

#include "log.h"
#include "navi.h"
#include "utils.h"

struct StreamDCtx {
    GMainLoop *loop;
    GstElement *pipeline;
    struct NaviCapture *navi_capture;
} g_ctx = {.loop = NULL, .pipeline = NULL, .navi_capture = NULL};

static gboolean on_bus_message(GstBus *bus, GstMessage *message, GMainLoop *loop) {
    GError *err;
    gchar *debug_info;
    switch (GST_MESSAGE_TYPE(message)) {
        case GST_MESSAGE_EOS:
            log_info("finished playback");
            g_main_loop_quit(loop);
            break;
        case GST_MESSAGE_ERROR:
            gst_message_parse_error(message, &err, &debug_info);
            log_error("error received from element %s: %s", GST_OBJECT_NAME(message->src), err->message);
            log_error("debugging information: %s", debug_info ? debug_info : "none");
            g_error_free(err);
            g_free(debug_info);
            g_main_loop_quit(loop);
            exit(1);
            break;
        default:
            break;
    }
    return TRUE;
}

gchar *compose_gstreamer_pipeline() {
    // based on: https://github.com/selkies-project/selkies-gstreamer/blob/main/src/selkies_gstreamer/gstwebrtc_app.py
    gchar *pipeline_str =
        "\
        ximagesrc display-name=%s show-pointer=%s use-damage=false remote=true blocksize=16384\
            ! video/x-raw,framerate=%s/1\
            ! videoconvert\
            ! textoverlay name=overlay valignment=center halignment=center font-desc=\"Sans, 16\"";
    if (VIDEO_ENC == "gpu-intel") {
        pipeline_str = g_strconcat(pipeline_str,
                                   "\
            ! qsvh264enc bitrate=10000 low-latency=true target-usage=7",
                                   NULL);
    } else if (VIDEO_ENC == "gpu-nvidia") {
        pipeline_str = g_strconcat(pipeline_str,
                                   "\
            ! cudaupload\
            ! cudaconvert qos=true\
            ! video/x-raw(memory:CUDAMemory),format=NV12\
            ! nvcudah264enc name=nvenc bitrate=10000 rate-control=cbr gop-size=-1 strict-gop=true aud=false b-adapt=false rc-lookahead=0 b-frames=0 zero-reorder-delay=true cabac=true repeat-sequence-header=true preset=p4 tune=ultra-low-latency multi-pass=two-pass-quarter",
                                   NULL);
    } else if (VIDEO_ENC == "cpu") {
        pipeline_str = g_strconcat(pipeline_str,
                                   "\
            ! x264enc bitrate=10000 tune=zerolatency speed-preset=ultrafast threads=2 key-int-max=2560 b-adapt=false bframes=0 b-pyramid=false vbv-buf-capacity=120 pass=cbr",
                                   NULL);
    } else {
        log_error("unrecognized VIDEO_ENC: %s", VIDEO_ENC);
        return NULL;
    }
    // constrained-baseline profile generates:
    // for cpu: na=fmtp:96 packetization-mode=1;profile-level-id=42c01f;level-asymmetry-allowed=1
    // for intel-gpu: na=fmtp:96 packetization-mode=1;profile-level-id=424028;level-asymmetry-allowed=1
    // which are compatible with both chrome and firefox
    // baseline and main profile generate profile-level-ids which are not supported in Firefox in cpu and/or intel-gpu
    // modes do not add stream-format=byte-stream into the caps as it breaks nvidia pipeline
    // (https://discourse.gstreamer.org/t/h264parse-never-receives-valid-picture-headers/3819)
    pipeline_str = g_strconcat(pipeline_str,
                               "\
            ! video/x-h264,profile=constrained-baseline \
            ! queue\
            ! wrs.\
        pulsesrc\
            ! queue\
            ! wrs.\
        webrtcsink\
            name=wrs\
            meta=\"meta,name=streamd,consumerId=(string)%s\"\
            enable-data-channel-navigation=true\
            congestion-control=disabled\
            stun-server=\"%s\"\
            signaller::insecure-tls=true\
            signaller::uri=\"%s\"\
            signaller::role=\"Producer\"\
            signaller::headers=\"headers,host=%s,cookie=\\\"sigsvc_wsconnid=%s; sigsvc_authtoken=%s\\\"\"",
                               NULL);
    return g_strdup_printf(pipeline_str, getenv("DISPLAY"), getenv("SHOW_POINTER"), getenv("FPS"),
                           getenv("WS_CONSUMER_ID"), getenv("STUN_URI"), getenv("SIGNALER_URI"),
                           getenv("SIGNALER_HOST"), getenv("WS_CONN_ID"), getenv("SIGNALER_AUTH_TOKEN"));
}

void play_stream() {
    log_info("play_stream");
    GError *error = NULL;
    gchar *pipeline_str = compose_gstreamer_pipeline();
    if (!pipeline_str) exit(1);
    log_info("pipeline: %s", pipeline_str);
    g_ctx.pipeline = gst_parse_launch(pipeline_str, &error);
    if (!g_ctx.pipeline) {
        log_error("parse error: %s", error->message);
    }
    GstBus *bus = gst_element_get_bus(g_ctx.pipeline);
    if (!bus) {
        log_error("bus error");
    }
    gst_bus_add_watch(bus, (GstBusFunc)on_bus_message, g_ctx.loop);
    gst_object_unref(bus);

    GstElement *src = gst_bin_get_by_name(GST_BIN(g_ctx.pipeline), "ximagesrc0");
    if (!src) {
        log_error("pipeline error");
    }
    GstPad *pad = gst_element_get_static_pad(src, "src");
    gst_pad_add_probe(pad, GST_PAD_PROBE_TYPE_EVENT_UPSTREAM, g_ctx.navi_capture->event_probe_cb, g_ctx.navi_capture,
                      NULL);

    GstElement *overlay = gst_bin_get_by_name(GST_BIN(g_ctx.pipeline), "overlay");
    if (!overlay) {
        log_error("Failed to get textoverlay element.\n");
    }
    CountdownArgs args = {.overlay = overlay, .seconds = atol(getenv("LOADING_PERIOD"))};
    pthread_t tid;
    pthread_create(&tid, NULL, countdown_overlay_thread, &args);

    gst_element_set_state(g_ctx.pipeline, GST_STATE_PLAYING);

    if (gst_element_get_state(g_ctx.pipeline, NULL, NULL, -1) == GST_STATE_CHANGE_FAILURE) {
        log_error("failed to go into PLAYING state");
    }
    return;
}

gint main(gint argc, gchar *argv[]) {
    setvbuf(stdout, NULL, _IONBF, 0);  // flush after printf

    // first we need to initialize navigation (mouse, keyboard) events handler
    // init_navi_capture() returns NaviCapture which we store and use later from the play_stream()
    struct NaviCapture *n_capture;
    if (!init_navi_capture(&n_capture)) log_error("navi events handler init error");
    g_ctx.navi_capture = n_capture;

    gst_init(&argc, &argv);
    g_ctx.loop = g_main_loop_new(NULL, FALSE);

    play_stream();

    log_info("running...");
    g_main_loop_run(g_ctx.loop);

    gst_element_set_state(g_ctx.pipeline, GST_STATE_NULL);
    gst_object_unref(g_ctx.pipeline);
    return 0;
}
