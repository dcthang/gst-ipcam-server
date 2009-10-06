/**
 * \file:   gst-ipcam-client-backend.c
 * \author: Dam Quang Tuan <damquang.tuan@nomovok.com>
 * \author: Nguyen Thanh Trung <nguyenthanh.trung@nomovok.com>
 *
 * \date 8-26-2009
 */
#include <string.h>

#include <gst/gst.h>
#include <gst/interfaces/xoverlay.h>

#include <gdk/gdkx.h>
#include <gdk/gdkkeysyms.h>

#include "gst-ipcam-client-backend.h"
#include "gst-ipcam-client-interface.h"
#include "gst-ipcam-client-callbacks.h"

static gpointer window;
static gchar *videoType;
static gchar *audioType;
static GstElement   *pipeline, *rtspsrc,
                    *v_depay_jpg, *v_decoder_jpg, *v_filter_jpg, *videosink,
                    *v_depay_h264, *v_decoder_h264, *v_filter_h264,
                    *v_depay_mp4, *v_decoder_mp4, *v_filter_mp4,
                    *a_depay_aac, *a_decoder_aac, *a_filter_aac, *a_resample_aac, *a_sink_aac,
                    *a_depay_g726, *a_decoder_g726, *a_filter_g726, *a_resample_g726, *a_sink_g726,
                    *a_depay_g711, *a_decoder_g711, *a_filter_g711, *a_resample_g711, *a_sink_g711;
static gint prewState;
static gint curtState;
/**
 * This function will try to find the best suitable video sink used for current window system.
 *
 * @return GstElement* a video sink that can be worked for current window system
 */
static GstElement * gst_ipcam_client_backend_find_best_video_sink();

/**
 * Compare ranks of 2 plugin features to sort the list of features.
 * Copied from GstAutoVideoSink source (gst-plugins-good-0.10.15)
 *
 * @param f1 GstPluginFeature* the first feature
 * @param f2 GstPluginFeature* the 2nd feature
 *
 * @return gint 0 if they are equal, -1 if f1 is less than f2 and 1 if f1 is greater
 */
static gint gst_ipcam_client_backend_video_sink_compare_ranks(GstPluginFeature * f1, GstPluginFeature * f2);

/**
 * Check if we should list given feature in to search list
 * Copied from GstAutoVideoSink source (gst-plugins-good-0.10.15)
 *
 * @param feature GstPluginFeature* feature to check
 * @param data gpointer userdata to callback function - unused
 *
 * @return gboolean TRUE if should/ FALSE if not
 */
static gboolean gst_ipcam_client_backend_video_sink_factory_filter(GstPluginFeature * feature, gpointer data);

/**
 * Create a sink with given factory
 *
 * @param factory GstElementFactory the factory to create element
 *
 * @return GstElement* an element created with given factory
 */
static GstElement * gst_ipcam_client_backend_create_sink(GstElementFactory * factory);

/**
 * handle messages from pipeline bus.
 *
 * @param bus GstBus* the bus that contains the messages
 * @param msg GstMessage* current bus message
 * @param data gpointer addition user data
 *
 * @return gboolean will comment on this later
 */
static gboolean gst_ipcam_client_backend_bus_watch(GstBus * bus, GstMessage * msg, gpointer data);

/**
 * Parse and print gstreamer message in the case that it's info/warning or error message.
 *
 * @param message GstMessage* the message that we need to parse and print
 *
 * @return Nothing
 */
static void gst_ipcam_client_backend_print_gst_message(GstMessage * message);

/**
 * init for using gstreamer
 * This function is used whenever the main function is created
 *
 * @param argc int *
 * @param argv[] char **
 *
 * @return nothing
 */
void
gst_ipcam_client_backend_init(int *argc,
		char **argv[]) {
	gst_init(argc, argv);
	pipeline = NULL;
}

/**
 * Set the pointer window
 *
 * @param window_ gpointer the pointer to show video region in the main window
 *
 * @return nothing
 */
void
gst_ipcam_client_backend_set_window(gpointer window_) {
	window = window_;
}

/**
 * set the pipeline from the state GST_STATE_NULL to the state GST_STATE_PLAYING
 *
 * @param nothing
 *
 * @return Result of the state change
 */
gint
gst_ipcam_client_backend_play() {
        prewState = GST_STOP_STATE;
        curtState = GST_PLAYING_STATE;
        GstStateChangeReturn stateReturn;

	stateReturn = gst_element_set_state(pipeline, GST_STATE_PLAYING);
	g_message("Setting to Play.....Done");
        
	return stateReturn;
}

/**
 * set the pipeline from the state GST_STATE_PLAYING to the state GST_STATE_PAUSED
 *
 * @param nothing
 *
 * @return Result of the state change
 */
gint
gst_ipcam_client_backend_pause() {
        prewState = GST_PLAYING_STATE;
        curtState = GST_PAUSE_STATE;
        
	GstStateChangeReturn stateReturn;

	stateReturn = gst_element_set_state(pipeline, GST_STATE_PAUSED);
	g_message("Setting to Pause.....Done");

	return stateReturn;
}

/**
 * set the pipeline from the state GST_STATE_PLAYING to the state GST_STATE_NULL
 *
 * @param nothing
 *
 * @return Result of the state change
 */
gint
gst_ipcam_client_backend_stop() {
        prewState = GST_PLAYING_STATE;
        curtState = GST_STOP_STATE;
        
	gtk_window_resize(GTK_WINDOW(mainWindow), 420, 50);
	GstStateChangeReturn stateReturn;

	stateReturn = gst_element_set_state(pipeline, GST_STATE_NULL);
	g_message("Setting to Stop.....Done");

        videoType = NULL;
        audioType = NULL;
	return stateReturn;
}

/**
 * set the pipeline from the state GST_STATE_PAUSED to the state GST_STATE_PLAYING
 *
 * @param nothing
 *
 * @return Result of the state change
 */
gint
gst_ipcam_client_backend_resume() {
        prewState = GST_PAUSE_STATE;
        curtState = GST_PLAYING_STATE;
        
	GstStateChangeReturn stateReturn;

	stateReturn = gst_element_set_state(pipeline, GST_STATE_PLAYING);
	g_message("Setting to Resume.....Done");

	return stateReturn;
}

/**
 * free memories for pipeline and videosink pointer before exit the application
 *
 * @param nothing
 *
 * @return nothing
 */
void
gst_ipcam_client_backend_deinit() {
	if (pipeline != NULL) {
		gst_object_unref(pipeline);
		pipeline = NULL;
	}
}

static GstElement * gst_ipcam_client_backend_find_best_video_sink() {
	GList *list, *item;
	GstElement *choice = NULL;
	GSList *errors = NULL;
	GstBus *bus = gst_bus_new();

	GstElement * sink = gst_element_factory_make("fakesink", "fake-video-sink");

	list = gst_registry_feature_filter(gst_registry_get_default(),
			(GstPluginFeatureFilter) gst_ipcam_client_backend_video_sink_factory_filter, FALSE, sink);
	list = g_list_sort(list, (GCompareFunc) gst_ipcam_client_backend_video_sink_compare_ranks);

	for (item = list; item != NULL; item = item->next) {
		GstElementFactory *f = GST_ELEMENT_FACTORY(item->data);
		GstElement *el;

		if ((el = gst_ipcam_client_backend_create_sink(f))) {
			GstStateChangeReturn ret;

			gst_element_set_bus(el, bus);
			ret = gst_element_set_state(el, GST_STATE_READY);
			if (ret == GST_STATE_CHANGE_SUCCESS) {
				choice = el;
				g_debug("We are using '%s'", GST_PLUGIN_FEATURE (f)->name);
				break;
			}

			/* collect all error messages */
/*
			while ((message = gst_bus_pop_filtered(bus, GST_MESSAGE_ERROR))) {
				g_warning("error message %" GST_PTR_FORMAT, message);
				errors = g_slist_append(errors, message);
			}
*/

			gst_element_set_state(el, GST_STATE_NULL);
			gst_object_unref(el);
		}
	}

	if (!choice) {
		if (errors) {
			/* FIXME: we forward the first error for now; but later on it might make
			 * sense to actually analyse them */
			gst_message_ref(GST_MESSAGE(errors->data));
			gst_ipcam_client_backend_print_gst_message(errors->data);
			gst_message_unref(GST_MESSAGE(errors->data));
		} else {
			/* send warning message to application and use a fakesink */
			g_warning("Failed to find a usable video sink");
			choice = sink;//gst_element_factory_make("fakesink", "fake-video-sink");
			if (g_object_class_find_property(G_OBJECT_GET_CLASS(choice), "sync"))
				g_object_set(choice, "sync", TRUE, NULL);
			gst_element_set_state(choice, GST_STATE_READY);
		}
	}
	gst_object_unref(bus);
	gst_plugin_feature_list_free(list);
	g_slist_foreach(errors, (GFunc) gst_mini_object_unref, NULL);
	g_slist_free(errors);

	return choice;
}

static gint gst_ipcam_client_backend_video_sink_compare_ranks(GstPluginFeature * f1, GstPluginFeature * f2) {
	gint diff;

	diff = gst_plugin_feature_get_rank(f2) - gst_plugin_feature_get_rank(f1);
	if (diff != 0)
		return diff;
	return strcmp(gst_plugin_feature_get_name(f2),
			gst_plugin_feature_get_name(f1));
}

static gboolean gst_ipcam_client_backend_video_sink_factory_filter(GstPluginFeature * feature, gpointer data) {
	guint rank;
	const gchar *klass;

	/* we only care about element factories */
	if (!GST_IS_ELEMENT_FACTORY(feature))
		return FALSE;

	/* video sinks */
	klass = gst_element_factory_get_klass(GST_ELEMENT_FACTORY(feature));
	if (!(strstr(klass, "Sink") && strstr(klass, "Video")))
		return FALSE;

	/* only select elements with autoplugging rank */
	rank = gst_plugin_feature_get_rank(feature);
	if (rank < GST_RANK_MARGINAL)
		return FALSE;

	return TRUE;
}

static GstElement * gst_ipcam_client_backend_create_sink(GstElementFactory* factory) {
	GstElement * sink;
	sink = gst_element_factory_create(factory, "video-sink");
	return sink;
}

static gboolean gst_ipcam_client_backend_bus_watch(GstBus* bus, GstMessage* msg, gpointer data) {
        switch (GST_MESSAGE_TYPE(msg)) {
            case GST_MESSAGE_ERROR:
            {
		gchar * debug;
		GError * error;
                gst_message_parse_error(msg, &error, &debug);
                if ((prewState == GST_PAUSE_STATE) && (curtState == GST_PLAYING_STATE))
                {
                    gst_ipcam_client_backend_stop();
                    pipeline = NULL;
                    gst_ipcam_client_backend_create_pipeline((gchar *)data);
                    gst_ipcam_client_backend_play();

                    g_message("PLAY request sent.");

                    /*Resize the mainwindow to show the video got from server*/
                    gtk_window_resize(GTK_WINDOW(mainWindow), 420, 400);
                }
                else
                {
                    g_message("PLAY request could not be sent.");
                
                    GtkWidget *dialog;
                    dialog = gtk_message_dialog_new(NULL,
                                                    GTK_DIALOG_DESTROY_WITH_PARENT,
                                                    GTK_MESSAGE_ERROR,
                                                    GTK_BUTTONS_CLOSE,
                                                    "The connection is failed. Please try again");

                    gtk_dialog_run (GTK_DIALOG (dialog));
                    gtk_widget_destroy (dialog);

                    gst_ipcam_client_on_btn_Disconnect_clicked(NULL, NULL);
                    g_free(debug);

                    g_warning("Pipeline error: %s", error->message);

                    g_error_free(error);
                
                }
            }
            break;
            case GST_MESSAGE_STATE_CHANGED:
            {
                GstState oldState, newState, pending;
                gst_message_parse_state_changed(msg, &oldState, &newState, &pending);
                switch (newState) {
                    case GST_STATE_PLAYING:
                        if (videosink != NULL)
                            gst_ipcam_client_read_video_props(videosink);
                        gst_ipcam_client_set_status_text("Playing");
                        gst_ipcam_client_set_status_Video_Type(videoType);
                        gst_ipcam_client_set_status_Audio_Type(audioType);
                        break;
                    case GST_STATE_PAUSED:
                        gst_ipcam_client_set_status_text("Paused");
                        break;
                    case GST_STATE_NULL:
                        gst_ipcam_client_set_status_text("Stopped");
                        break;
                    case GST_STATE_READY:
                        break;
                    default:
                        g_message("Unknow state : %d!", newState);
                        gst_ipcam_client_set_status_text("Stopped");
                        break;
                }
                break;
            }
                break;
            default:
		break;
        }
	return TRUE;
}

static void gst_ipcam_client_backend_print_gst_message(GstMessage* message) {
	switch (GST_MESSAGE_TYPE(message)) {
		case GST_MESSAGE_ERROR:
		case GST_MESSAGE_INFO:
		case GST_MESSAGE_WARNING:
		{
			gchar * debug;
			GError * error;
			gst_message_parse_error(message, &error, &debug);

			if (debug != NULL) {
				g_free(debug);
			}

			if (error != NULL) {
				g_warning("Backend message (%s): %s", GST_MESSAGE_TYPE_NAME(message), error->message);
				g_error_free(error);
			}
		}
			break;
		default:
			break;
	}
}

/**
 * Handle the event when the signal pad-added is called
 *
 * @param element GstElement *
 * @param pad GstPad *
 *
 * @return nothing
 */
void gst_ipcam_client_on_pad_added (GstElement *element, GstPad *pad)
{
    g_debug ("Signal: pad-added");
    GstCaps *caps;
    GstStructure *str;
    caps = gst_pad_get_caps (pad);
    g_assert (caps != NULL);

    str = gst_caps_get_structure (caps, 0);
    g_assert (str != NULL);

    const gchar *c = gst_structure_get_string(str, "media");
    const gchar *stream_type = gst_structure_get_string(str, "encoding-name");
    
    g_message("Stream type : %s\n", stream_type);
    g_message("Pad name %s\n", c);
    gst_element_set_state (pipeline, GST_STATE_PAUSED);
    if (g_strrstr (c, "video"))
    {
        videosink = gst_ipcam_client_backend_find_best_video_sink();
    	if (g_strrstr (stream_type, "H264"))
        {
            videoType = g_strconcat("", "Video type: H264", NULL);
            
            v_depay_h264 = gst_element_factory_make ("rtph264depay", "depay_h264");

            v_decoder_h264 = gst_element_factory_make ("ffdec_h264", "decoder_h264");

            v_filter_h264 = gst_element_factory_make ("ffmpegcolorspace", "ffmpegcsp_h264");

            gst_bin_add_many (GST_BIN (pipeline), v_depay_h264, v_decoder_h264, v_filter_h264, videosink, NULL);
            (gst_element_link (v_depay_h264, v_decoder_h264));
            (gst_element_link (v_decoder_h264, v_filter_h264));
            (gst_element_link (v_filter_h264, videosink));
            g_debug ("Linking video pad %s", stream_type);
            /* Link it actually*/
            GstPad *targetsink = gst_element_get_pad (v_depay_h264, "sink");
            g_assert (targetsink != NULL);
            gst_pad_link (pad, targetsink);
            gst_object_unref (targetsink);
    	}
        else if (g_strrstr (stream_type, "MP4V-ES"))
        {
            videoType = g_strconcat("", "Video type: MPEG4", NULL);

            v_depay_mp4 = gst_element_factory_make ("rtpmp4vdepay", "depay_mp4");

            v_decoder_mp4 = gst_element_factory_make ("ffdec_mpeg4", "decoder_mp4");

            v_filter_mp4 = gst_element_factory_make ("ffmpegcolorspace", "ffmpegcsp_mp4");

            gst_bin_add_many (GST_BIN (pipeline), v_depay_mp4, v_decoder_mp4, v_filter_mp4, videosink, NULL);

            (gst_element_link (v_depay_mp4, v_decoder_mp4));
            (gst_element_link (v_decoder_mp4, v_filter_mp4));
            (gst_element_link (v_filter_mp4, videosink));
            g_debug ("Linking video pad %s", stream_type);
            /* Link it actually*/
            GstPad *targetsink = gst_element_get_pad (v_depay_mp4, "sink");
            g_assert (targetsink != NULL);
            gst_pad_link (pad, targetsink);
            gst_object_unref (targetsink);
    	}
        else if (g_strrstr (stream_type, "JPEG"))
        {
            videoType = g_strconcat("", "Video type: JPEG", NULL);

            v_depay_jpg = gst_element_factory_make ("rtpjpegdepay", "depay_jpg");

            v_decoder_jpg = gst_element_factory_make ("jpegdec", "decoder_jpg");
            v_filter_jpg = gst_element_factory_make ("ffmpegcolorspace", "ffmpegcsp_jpg");

            gst_bin_add_many (GST_BIN (pipeline), v_depay_jpg, v_decoder_jpg, v_filter_jpg, videosink, NULL);

            (gst_element_link (v_depay_jpg, v_decoder_jpg));
            (gst_element_link (v_decoder_jpg, v_filter_jpg));
            (gst_element_link (v_filter_jpg, videosink));

            g_debug ("Linking video pad %s", stream_type);
            /* Link it actually*/
            GstPad *targetsink = gst_element_get_pad (v_depay_jpg, "sink");
            g_assert (targetsink != NULL);
            gst_pad_link (pad, targetsink);
            gst_object_unref (targetsink);
    	}
    }
    if (g_strrstr (c, "audio"))
    {
    	if (g_strrstr (stream_type, "G726-16"))
        {
            audioType = g_strconcat("", "/Audio type: G726", NULL);

            a_depay_g726 = gst_element_factory_make ("rtpg726depay", "adepay-g726");
            a_decoder_g726 = gst_element_factory_make ("ffdec_g726", "adecoder-g726");
            a_filter_g726 = gst_element_factory_make ("audioconvert", "audioconvert-g726");
            a_resample_g726 = gst_element_factory_make("audioresample", "audioresample-g726");
            a_sink_g726     = gst_element_factory_make ("autoaudiosink", "audio sink g726");
            gst_bin_add_many (GST_BIN (pipeline), a_depay_g726, a_decoder_g726, a_filter_g726, a_resample_g726, a_sink_g726, NULL);
            (gst_element_link (a_depay_g726, a_decoder_g726));
            (gst_element_link (a_decoder_g726, a_filter_g726));
            (gst_element_link (a_filter_g726, a_resample_g726));
            (gst_element_link (a_resample_g726, a_sink_g726));
            g_debug ("Linking audio pad %s", stream_type);
            /* Link it actually*/
            GstPad *targetsink = gst_element_get_pad (a_depay_g726, "sink");
            g_assert (targetsink != NULL);
            gst_pad_link (pad, targetsink);
            gst_object_unref (targetsink);
    	}
        else if (g_strrstr (stream_type, "PCMA"))
        {
            audioType = g_strconcat("", "/Audio type: G711", NULL);
            
            a_depay_g711 = gst_element_factory_make ("rtppcmadepay", "adepay-g711");
            a_decoder_g711 = gst_element_factory_make ("alawdec", "adecoder-g711");
            a_filter_g711 = gst_element_factory_make ("audioconvert", "audioconvert-g711");
            a_resample_g711 = gst_element_factory_make("audioresample", "audioresample-g711");
            a_sink_g711     = gst_element_factory_make ("autoaudiosink", "audio sink g711");

            gst_bin_add_many (GST_BIN (pipeline), a_depay_g711, a_decoder_g711, a_filter_g711, a_resample_g711, a_sink_g711, NULL);

            (gst_element_link (a_depay_g711, a_decoder_g711));
            (gst_element_link (a_decoder_g711, a_filter_g711));
            (gst_element_link (a_filter_g711, a_resample_g711));
            (gst_element_link (a_resample_g711, a_sink_g711));
            g_debug ("Linking audio pad %s", stream_type);
            // Link it actually
            GstPad *targetsink = gst_element_get_pad (a_depay_g711, "sink");
            g_assert (targetsink != NULL);
            gst_pad_link (pad, targetsink);
            gst_object_unref (targetsink);
    	}
        else if (g_strrstr (stream_type, "MP4A-LATM"))
        {
            audioType = g_strconcat("", "/Audio type: AAC", NULL);
            
            a_depay_aac = gst_element_factory_make ("rtpmp4adepay", "adepay_aac");
            a_decoder_aac = gst_element_factory_make ("faad", "adecoder_aac");
            a_filter_aac = gst_element_factory_make ("audioconvert", "audioconvert_aac");
            a_resample_aac = gst_element_factory_make("audioresample", "audioresample_aac");
            a_sink_aac     = gst_element_factory_make ("autoaudiosink", "audio sink aac");

            gst_bin_add_many (GST_BIN (pipeline), a_depay_aac, a_decoder_aac, a_filter_aac,
                              a_resample_aac, a_sink_aac, NULL);

            (gst_element_link (a_depay_aac, a_decoder_aac));
            (gst_element_link (a_decoder_aac, a_filter_aac));
            (gst_element_link (a_filter_aac, a_resample_aac));
            (gst_element_link (a_resample_aac, a_sink_aac));

            g_debug ("Linking audio pad %s", stream_type);
            // Link it actually
            GstPad *targetsink = gst_element_get_pad (a_depay_aac, "sink");
            g_assert (targetsink != NULL);
            gst_pad_link (pad, targetsink);
            gst_object_unref (targetsink);
    	}
    }
    gst_element_set_state (pipeline, GST_STATE_PLAYING);
    g_object_set(G_OBJECT(videosink), "force-aspect-ratio", TRUE, NULL);

    if (GST_IS_X_OVERLAY(videosink))
    {
	gst_x_overlay_set_xwindow_id(GST_X_OVERLAY(videosink), GPOINTER_TO_INT(window));
    }
    gst_caps_unref (caps);
}

/**
 * create the pipeline to get the data from the rtsp server
 *
 * @param url const gchar * the url of server
 *
 * @return nothing
 */
void
gst_ipcam_client_backend_create_pipeline(const gchar *url)
{
    GstBus *bus;
    /* create elements */
    pipeline = gst_pipeline_new ("pipeline");
    /* watch for messages on the pipeline's bus (note that this will only
    * work like this when a GLib main loop is running) */
    bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
    gst_bus_add_watch (bus, gst_ipcam_client_backend_bus_watch, g_strdup(url));
    gst_object_unref (bus);
    
    rtspsrc = gst_element_factory_make ("rtspsrc", "rtspsource");

    g_object_set (G_OBJECT (rtspsrc), "location", url, NULL);
    g_object_set (G_OBJECT (rtspsrc), "debug", TRUE, NULL);
    g_signal_connect (rtspsrc, "pad-added", G_CALLBACK (gst_ipcam_client_on_pad_added), NULL);

    gst_bin_add_many (GST_BIN (pipeline), rtspsrc, NULL);
}

/**
 * Read the video properties
 *
 * @param videosink GstElement *
 *
 * @return nothing
 */
void
gst_ipcam_client_read_video_props (GstElement *videosink)
{
    gint fps_n, fps_d;
    gint width, height;
    gint datarate;
    gchar *capsInfor;
    GstStructure *str = NULL;
    GstPad *pad;
    GstCaps *caps;

    pad = gst_element_get_static_pad(videosink, "sink");
    if (pad == NULL)
    {
        return;
    }
    caps = gst_pad_get_negotiated_caps (pad);

    g_return_if_fail (gst_caps_is_fixed (caps));
    str = gst_caps_get_structure (caps, 0);

    if (str == NULL)
    {
        return;
    }

    capsInfor = gst_caps_to_string (caps);
    g_message ("caps Infor: %s", capsInfor);
    
    gst_structure_get_fraction(str, "framerate", &fps_n, &fps_d);
    gst_structure_get_int(str, "width", &width);
    gst_structure_get_int(str, "height", &height);

    gst_structure_get_int(str, "datarate", &datarate);

    g_message("The datarate is : %d\n", datarate);

    g_message ("frame rate %d/%d", fps_n, fps_d);
    g_message ("The video size of this set of capabilities is %dx%d",
    width, height);
}