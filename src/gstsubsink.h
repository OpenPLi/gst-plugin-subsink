/* GStreamer
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Library General Public
* License as published by the Free Software Foundation; either
* version 2 of the License, or (at your option) any later version.
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

#ifndef _GST_SUB_SINK_H_
#define _GST_SUB_SINK_H_

#include <gst/gst.h>
#include <gst/base/gstbasesink.h>

G_BEGIN_DECLS

#define GST_TYPE_SUB_SINK \
	(gst_sub_sink_get_type())
#define GST_SUB_SINK(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_SUB_SINK,GstSubSink))
#define GST_SUB_SINK_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_SUB_SINK,GstSubSinkClass))
#define GST_IS_SUB_SINK(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_SUB_SINK))
#define GST_IS_SUB_SINK_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_SUB_SINK))
/* Since 0.10.23 */
#define GST_SUB_SINK_CAST(obj) \
	((GstSubSink*)(obj))

typedef struct _GstSubSink GstSubSink;
typedef struct _GstSubSinkClass GstSubSinkClass;
typedef struct _GstSubSinkPrivate GstSubSinkPrivate;

struct _GstSubSink
{
	GstBaseSink basesink;

	/*< private >*/
	GstSubSinkPrivate *priv;

	/*< private >*/
	gpointer     _gst_reserved[GST_PADDING];
};

struct _GstSubSinkClass
{
	GstBaseSinkClass basesink_class;

	/* signals */
	void        (*new_buffer)   (GstSubSink *sink, GstBuffer *buffer);

	/*< private >*/
	gpointer     _gst_reserved[GST_PADDING - 2];
};

GType gst_sub_sink_get_type(void);

void            gst_sub_sink_set_caps         (GstSubSink *subsink, const GstCaps *caps);
GstCaps *       gst_sub_sink_get_caps         (GstSubSink *subsink);

G_END_DECLS

#endif

