glib_enum_prefix = gst_subsink


BUILT_SOURCES = $(built_sources) $(built_headers)
# plugindir is set in configure

CLEANFILES = $(BUILT_SOURCES)

plugin_LTLIBRARIES = libgstsubsink.la

# for the next set of variables, rename the prefix if you renamed the .la

# sources used to compile this plug-in
libgstsubsink_la_SOURCES = gstsubsink.c

# flags used to compile this plugin
# add other _CFLAGS and _LIBS as needed

libgstsubsink_la_CFLAGS = $(GST_CFLAGS)
libgstsubsink_la_LIBADD = $(GST_LIBS) -lgstbase-$(GST_MAJORMINOR)
libgstsubsink_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)

# headers we need but don't want installed
noinst_HEADERS = gstsubsink.h
