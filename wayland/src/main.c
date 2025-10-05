#include <fcntl.h>
#include <gst/gst.h>
#include <util/bmem.h>

#include "./navi/navi.h"
#include "./portal/screencast-portal.h"
#include "utils.h"

struct StreamDCtx {
    GMainLoop *loop;
    GstElement *pipeline;
    struct NaviCapture *navi_capture;
} g_ctx = {.loop = NULL, .pipeline = NULL, .navi_capture = NULL};

static gboolean on_bus_message(GstBus *bus, GstMessage *message, GMainLoop *loop) {
    switch (GST_MESSAGE_TYPE(message)) {
        case GST_MESSAGE_EOS:
            log_info("finished playback");
            g_main_loop_quit(loop);
            break;
        case GST_MESSAGE_ERROR:
            GError *err;
            gchar *debug_info;
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

gchar *compose_gstreamer_pipeline(int pipewire_fd) {
    gchar *pipeline_str =
        "\
        pipewiresrc fd=%u\
            ! videoconvert\
            ! video/x-raw,format=NV12,framerate=0/1,max-framerate=%s/1";
    if (VIDEO_ENC == "gpu-intel") {
        pipeline_str = g_strconcat(pipeline_str,
                                   "\
            ! qsvh264enc bitrate=10000 low-latency=true target-usage=7",
                                   NULL);

    } else if (VIDEO_ENC == "cpu") {
        pipeline_str = g_strconcat(pipeline_str,
                                   "\
            ! x264enc bitrate=10000 tune=zerolatency speed-preset=ultrafast threads=2 key-int-max=2560 b-adapt=false bframes=0 b-pyramid=false vbv-buf-capacity=120 pass=cbr",
                                   NULL);
    } else {
        return NULL;
    }
    pipeline_str = g_strconcat(pipeline_str,
                               "\
            ! video/x-h264,profile=baseline\
            ! textoverlay name=overlay valignment=center halignment=center font-desc=\"Sans, 16\"\
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
    return g_strdup_printf(pipeline_str, pipewire_fd, getenv("FPS"), getenv("WS_CONSUMER_ID"), getenv("STUN_URI"),
                           getenv("SIGNALER_URI"), getenv("SIGNALER_HOST"), getenv("WS_CONN_ID"),
                           getenv("SIGNALER_AUTH_TOKEN"));
}

void on_pipewire_stream_ready_cb(int pipewire_fd) {
    log_info("on_pipewire_stream_ready_cb");
    GError *error = NULL;
    // pipeline_descr = "videotestsrc ! fakesink";
    /* ! vah264enc bitrate=10000\
       ! video/x-h264,profile=baseline\
    */
    gchar *pipeline_str = compose_gstreamer_pipeline(pipewire_fd);
    if (!pipeline_str) exit(1);
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

    GstElement *pipewiresrc = gst_bin_get_by_name(GST_BIN(g_ctx.pipeline), "pipewiresrc0");
    if (!pipewiresrc) {
        log_error("pipeline error");
    }
    GstPad *pad = gst_element_get_static_pad(pipewiresrc, "src");
    gst_pad_add_probe(pad, GST_PAD_PROBE_TYPE_EVENT_UPSTREAM, g_ctx.navi_capture->event_probe_cb, g_ctx.navi_capture,
                      NULL);
    
    GstElement *overlay = gst_bin_get_by_name(GST_BIN(g_ctx.pipeline), "overlay");
    if (!overlay) {
        log_error("Failed to get textoverlay element.\n");
    }
    CountdownArgs args = {.overlay = overlay, .seconds = 5};
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
    // init_navi_capture() returns NaviCapture which we use later from the on_pipewire_stream_ready_cb()
    struct NaviCapture *n_capture;
    if (!init_navi_capture(&n_capture)) log_error("navi events handler init error");
    g_ctx.navi_capture = n_capture;

    // next we initialize pipewire screencast stream
    // we pass a ref to on_pipewire_stream_ready_cb() into init_screencast_capture() so it is called at the end
    struct ScreencastPortalCapture *scp_capture;
    scp_capture = bzalloc(sizeof(struct ScreencastPortalCapture));
    scp_capture->capture_type = PORTAL_CAPTURE_TYPE_MONITOR;
    scp_capture->cursor_visible = true;
    scp_capture->restore_token = NULL;
    scp_capture->stream_ready_cb = on_pipewire_stream_ready_cb;
    if (!init_screencast_capture(scp_capture)) log_error("screencast init error");

    gst_init(&argc, &argv);
    g_ctx.loop = g_main_loop_new(NULL, FALSE);
    log_info("running...");
    g_main_loop_run(g_ctx.loop);

    bfree(scp_capture);
    gst_element_set_state(g_ctx.pipeline, GST_STATE_NULL);
    gst_object_unref(g_ctx.pipeline);
    return 0;
}
