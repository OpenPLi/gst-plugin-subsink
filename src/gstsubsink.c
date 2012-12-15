/* GStreamer
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Library General Public
* License as published by the Free Software Foundation; either
* version 2 of the License, or(at your option) any later version.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Library General Public License for more details.
*
* You should have received a copy of the GNU Library General Public
* License along with this library; if not, write to the
* Free Software Foundation, Inc., 59 Temple Place - Suite 330,
* Boston, MA 02111-1307, USA.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include <gst/base/gstbasesink.h>
#include <gst/gstbuffer.h>
#include <gst/gstbufferlist.h>

#include <string.h>

#include "gstsubsink-marshal.h"
#include "gstsubsink.h"

struct _GstSubSinkPrivate
{
	GstCaps *caps;

	gboolean flushing;

	gpointer user_data;
	GDestroyNotify notify;
};

GST_DEBUG_CATEGORY_STATIC(sub_sink_debug);
#define GST_CAT_DEFAULT sub_sink_debug

enum
{
	/* signals */
	SIGNAL_NEW_BUFFER,

	LAST_SIGNAL
};

enum
{
	PROP_0,
	PROP_CAPS,
	PROP_LAST
};

static GstStaticPadTemplate gst_sub_sink_template =
GST_STATIC_PAD_TEMPLATE("sink",
		GST_PAD_SINK,
		GST_PAD_ALWAYS,
		GST_STATIC_CAPS_ANY);

static void gst_sub_sink_uri_handler_init(gpointer g_iface,
		gpointer iface_data);

#if GST_VERSION_MAJOR < 1
static void gst_sub_sink_init(GstSubSink *subsink, GstSubSinkClass *klass);
#else
static void gst_sub_sink_init(GstSubSink *subsink);
#endif
static void gst_sub_sink_dispose(GObject *object);

static void gst_sub_sink_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
static void gst_sub_sink_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);

static gboolean gst_sub_sink_start(GstBaseSink *psink);
static gboolean gst_sub_sink_stop(GstBaseSink *psink);
static GstFlowReturn gst_sub_sink_render_common(GstBaseSink *psink, GstBuffer *buffer, gboolean is_list);
static GstFlowReturn gst_sub_sink_render(GstBaseSink *psink, GstBuffer *buffer);
static GstFlowReturn gst_sub_sink_render_list(GstBaseSink *psink, GstBufferList *list);
#if GST_VERSION_MAJOR < 1
static GstCaps *gst_sub_sink_getcaps(GstBaseSink *psink);
#else
static GstCaps *gst_sub_sink_getcaps(GstBaseSink *psink, GstCaps *filter);
#endif

static guint gst_sub_sink_signals[LAST_SIGNAL] = { 0 };

#if GST_VERSION_MAJOR < 1
static void _do_init(GType filesrc_type)
{
	static const GInterfaceInfo urihandler_info = 
	{
		gst_sub_sink_uri_handler_init,
		NULL,
		NULL
	};
	g_type_add_interface_static(filesrc_type, GST_TYPE_URI_HANDLER, &urihandler_info);
}

static void gst_sub_sink_base_init(gpointer g_class)
{
	GstElementClass *element_class = GST_ELEMENT_CLASS(g_class);

	GST_DEBUG_CATEGORY_INIT(sub_sink_debug, "subsink", 0, "subsink element");

	gst_element_class_set_details_simple(element_class, "SubSink",
			"Generic/Sink", "Allow the application to get access to raw subtitle data",
			"PLi team");

	gst_element_class_add_pad_template(element_class,
			gst_static_pad_template_get(&gst_sub_sink_template));
}

GST_BOILERPLATE_FULL(GstSubSink, gst_sub_sink, GstBaseSink, GST_TYPE_BASE_SINK, _do_init);
#else
#define gst_sub_sink_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE(GstSubSink, gst_sub_sink, GST_TYPE_BASE_SINK, G_IMPLEMENT_INTERFACE(GST_TYPE_URI_HANDLER, gst_sub_sink_uri_handler_init));
#endif

static void gst_sub_sink_class_init(GstSubSinkClass * klass)
{
	GObjectClass *gobject_class =(GObjectClass *) klass;
	GstBaseSinkClass *basesink_class =(GstBaseSinkClass *) klass;

#if GST_VERSION_MAJOR >= 1
	GstElementClass *element_class = (GstElementClass *)klass;

	GST_DEBUG_CATEGORY_INIT(sub_sink_debug, "subsink", 0, "subsink element");

	gst_element_class_set_static_metadata(element_class, "SubSink",
			"Generic/Sink", "Allow the application to get access to raw subtitle data",
			"PLi team");

	gst_element_class_add_pad_template(element_class,
			gst_static_pad_template_get(&gst_sub_sink_template));
#endif

	gobject_class->dispose = gst_sub_sink_dispose;

	gobject_class->set_property = gst_sub_sink_set_property;
	gobject_class->get_property = gst_sub_sink_get_property;

	g_object_class_install_property(gobject_class, PROP_CAPS,
			g_param_spec_boxed("caps", "Caps",
					"The allowed caps for the sink pad", GST_TYPE_CAPS,
					G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	* GstSubSink::new-buffer:
	* @subsink: the subsink element that emited the signal
	*
	* Signal that a new buffer is available.
	*
	* This signal is emited from the steaming thread
	*
	*/
	gst_sub_sink_signals[SIGNAL_NEW_BUFFER] =
			g_signal_new("new-buffer", G_TYPE_FROM_CLASS(klass), G_SIGNAL_RUN_LAST,
			G_STRUCT_OFFSET(GstSubSinkClass, new_buffer),
			NULL, NULL, gst_subsink_marshal_VOID__POINTER, G_TYPE_NONE, 1, G_TYPE_POINTER);

	basesink_class->start = gst_sub_sink_start;
	basesink_class->stop = gst_sub_sink_stop;
	basesink_class->render = gst_sub_sink_render;
	basesink_class->render_list = gst_sub_sink_render_list;
	basesink_class->get_caps = gst_sub_sink_getcaps;

	g_type_class_add_private(klass, sizeof(GstSubSinkPrivate));
}

#if GST_VERSION_MAJOR < 1
static void gst_sub_sink_init(GstSubSink *subsink, GstSubSinkClass *klass)
#else
static void gst_sub_sink_init(GstSubSink *subsink)
#endif
{
	subsink->priv =
			G_TYPE_INSTANCE_GET_PRIVATE(subsink, GST_TYPE_SUB_SINK,
			GstSubSinkPrivate);
}

static void gst_sub_sink_dispose(GObject *obj)
{
	GstSubSink *subsink = GST_SUB_SINK_CAST(obj);
	GstSubSinkPrivate *priv = subsink->priv;

	GST_OBJECT_LOCK(subsink);
	if (priv->caps)
	{
		gst_caps_unref(priv->caps);
		priv->caps = NULL;
	}
	if (priv->notify) 
	{
		priv->notify(priv->user_data);
	}
	priv->user_data = NULL;
	priv->notify = NULL;

	GST_OBJECT_UNLOCK(subsink);

	G_OBJECT_CLASS(parent_class)->dispose(obj);
}

static void gst_sub_sink_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	GstSubSink *subsink = GST_SUB_SINK_CAST(object);

	switch(prop_id) 
	{
		case PROP_CAPS:
			gst_sub_sink_set_caps(subsink, gst_value_get_caps(value));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
			break;
	}
}

static void gst_sub_sink_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	GstSubSink *subsink = GST_SUB_SINK_CAST(object);

	switch(prop_id) 
	{
		case PROP_CAPS:
		{
			GstCaps *caps;

			caps = gst_sub_sink_get_caps(subsink);
			gst_value_set_caps(value, caps);
			if (caps)
				gst_caps_unref(caps);
			break;
		}
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
			break;
	}
}

static gboolean gst_sub_sink_start(GstBaseSink *psink)
{
	GstSubSink *subsink = GST_SUB_SINK_CAST(psink);
	GstSubSinkPrivate *priv = subsink->priv;

	GST_DEBUG_OBJECT(subsink, "starting");
	priv->flushing = FALSE;

	return TRUE;
}

static gboolean gst_sub_sink_stop(GstBaseSink *psink)
{
	GstSubSink *subsink = GST_SUB_SINK_CAST(psink);
	GstSubSinkPrivate *priv = subsink->priv;

	GST_DEBUG_OBJECT(subsink, "stopping");
	priv->flushing = TRUE;

	return TRUE;
}

static GstFlowReturn gst_sub_sink_render_common(GstBaseSink *psink, GstBuffer *buffer, gboolean is_list)
{
	GstSubSink *subsink = GST_SUB_SINK_CAST(psink);
	GstSubSinkPrivate *priv = subsink->priv;

	if (priv->flushing)
		goto flushing;

	g_signal_emit(subsink, gst_sub_sink_signals[SIGNAL_NEW_BUFFER], 0, gst_buffer_ref(buffer));

	return GST_FLOW_OK;

flushing:
	{
		GST_DEBUG_OBJECT(subsink, "we are flushing");
#if GST_VERSION_MAJOR < 1
		return GST_FLOW_WRONG_STATE;
#else
		return GST_FLOW_FLUSHING;
#endif
	}
}

static GstFlowReturn gst_sub_sink_render(GstBaseSink *psink, GstBuffer *buffer)
{
	return gst_sub_sink_render_common(psink, buffer, FALSE);
}

static GstFlowReturn gst_sub_sink_render_list(GstBaseSink *sink, GstBufferList *list)
{
#if GST_VERSION_MAJOR < 1
	GstBufferListIterator *it;
	GstBuffer *group;
#else
	GstBuffer *buffer;
	guint i, len;
#endif
	GstFlowReturn flow;

	/* The application doesn't support buffer lists, extract individual buffers
	* then and push them one-by-one */
	GST_INFO_OBJECT(sink, "chaining each group in list as a merged buffer");

#if GST_VERSION_MAJOR < 1
	it = gst_buffer_list_iterate(list);

	if (gst_buffer_list_iterator_next_group(it)) 
	{
		do 
		{
			group = gst_buffer_list_iterator_merge_group(it);
			if (group == NULL) 
			{
				group = gst_buffer_new();
				GST_DEBUG_OBJECT(sink, "chaining empty group");
			} 
			else 
			{
				GST_DEBUG_OBJECT(sink, "chaining group");
			}
			flow = gst_sub_sink_render(sink, group);
			gst_buffer_unref(group);
		} while (flow == GST_FLOW_OK && gst_buffer_list_iterator_next_group(it));
	} 
	else 
	{
		GST_DEBUG_OBJECT(sink, "chaining empty group");
		group = gst_buffer_new();
		flow = gst_sub_sink_render(sink, group);
		gst_buffer_unref(group);
	}

	gst_buffer_list_iterator_free(it);
#else
	len = gst_buffer_list_length(list);
	flow = GST_FLOW_OK;
	for (i = 0; i < len; i++)
	{
		buffer = gst_buffer_list_get(list, i);
		flow = gst_sub_sink_render(sink, buffer);
		if (flow != GST_FLOW_OK)
		{
			break;
		}
	}
#endif

	return flow;
}

#if GST_VERSION_MAJOR < 1
static GstCaps *gst_sub_sink_getcaps(GstBaseSink *psink)
#else
static GstCaps *gst_sub_sink_getcaps(GstBaseSink *psink, GstCaps *filter)
#endif
{
	GstCaps *caps;
	GstSubSink *subsink = GST_SUB_SINK_CAST(psink);
	GstSubSinkPrivate *priv = subsink->priv;

	GST_OBJECT_LOCK(subsink);
	if ((caps = priv->caps))
	{
#if GST_VERSION_MAJOR < 1
		gst_caps_ref(caps);
#else
		if (filter)
		{
			caps = gst_caps_intersect_full(filter, caps, GST_CAPS_INTERSECT_FIRST);
		}
		else
		{
			gst_caps_ref(caps);
		}
#endif
	}
	GST_DEBUG_OBJECT(subsink, "got caps %" GST_PTR_FORMAT, caps);
	GST_OBJECT_UNLOCK(subsink);

	return caps;
}

/* external API */

void gst_sub_sink_set_caps(GstSubSink *subsink, const GstCaps *caps)
{
	GstCaps *old;
	GstSubSinkPrivate *priv;

	g_return_if_fail(GST_IS_SUB_SINK(subsink));

	priv = subsink->priv;

	GST_OBJECT_LOCK(subsink);
	GST_DEBUG_OBJECT(subsink, "setting caps to %" GST_PTR_FORMAT, caps);
	if ((old = priv->caps) != caps) 
	{
		if (caps)
			priv->caps = gst_caps_copy(caps);
		else
			priv->caps = NULL;
		if (old)
			gst_caps_unref(old);
	}
	GST_OBJECT_UNLOCK(subsink);
}

GstCaps *gst_sub_sink_get_caps(GstSubSink *subsink)
{
	GstCaps *caps;
	GstSubSinkPrivate *priv;

	g_return_val_if_fail(GST_IS_SUB_SINK(subsink), NULL);

	priv = subsink->priv;

	GST_OBJECT_LOCK(subsink);
	if ((caps = priv->caps))
		gst_caps_ref(caps);
	GST_DEBUG_OBJECT(subsink, "getting caps of %" GST_PTR_FORMAT, caps);
	GST_OBJECT_UNLOCK(subsink);

	return caps;
}

/*** GSTURIHANDLER INTERFACE *************************************************/

#if GST_VERSION_MAJOR < 1
static GstURIType gst_sub_sink_uri_get_type(void)
#else
static GstURIType gst_sub_sink_uri_get_type(GType type)
#endif
{
	return GST_URI_SINK;
}

#if GST_VERSION_MAJOR < 1
static gchar **gst_sub_sink_uri_get_protocols(void)
{
	static gchar *protocols[] = {(char *) "subsink", NULL };
#else
static const gchar *const *gst_sub_sink_uri_get_protocols(GType type)
{
	static const gchar *protocols[] = {"subsink", NULL};
#endif
	return protocols;
}

#if GST_VERSION_MAJOR < 1
static const gchar *gst_sub_sink_uri_get_uri(GstURIHandler *handler)
{
	return "subsink";
}
#else
static gchar *gst_sub_sink_uri_get_uri(GstURIHandler *handler)
{
	return g_strdup("subsink");
}
#endif

#if GST_VERSION_MAJOR < 1
static gboolean gst_sub_sink_uri_set_uri(GstURIHandler *handler, const gchar *uri)
{
	gchar *protocol;
	gboolean ret;

	protocol = gst_uri_get_protocol(uri);
	ret = !strcmp(protocol, "subsink");
	g_free(protocol);

	return ret;
}
#else
static gboolean gst_sub_sink_uri_set_uri(GstURIHandler *handler, const gchar *uri, GError **error)
{
	/* GstURIHandler checks the protocol for us */
	return TRUE;
}
#endif

static void gst_sub_sink_uri_handler_init(gpointer g_iface, gpointer iface_data)
{
	GstURIHandlerInterface *iface =(GstURIHandlerInterface *) g_iface;

	iface->get_type = gst_sub_sink_uri_get_type;
	iface->get_protocols = gst_sub_sink_uri_get_protocols;
	iface->get_uri = gst_sub_sink_uri_get_uri;
	iface->set_uri = gst_sub_sink_uri_set_uri;
}

static gboolean plugin_init(GstPlugin *plugin)
{
	if (!gst_element_register(plugin, "subsink", GST_RANK_PRIMARY, GST_TYPE_SUB_SINK))
		return FALSE;

	return TRUE;
}

GST_PLUGIN_DEFINE(GST_VERSION_MAJOR,
		GST_VERSION_MINOR,
#if GST_VERSION_MAJOR < 1
		"subsink",
#else
		subsink,
#endif
		"delivers subtitle buffers to the application",
		plugin_init, VERSION, "LGPL", "GStreamer", "http://gstreamer.net/");
