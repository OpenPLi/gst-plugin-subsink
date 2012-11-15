glib_enum_prefix = gst_subsink

include glib-gen.mak

built_sources = gstsubsink-marshal.c
built_headers = gstsubsink-marshal.h

BUILT_SOURCES = $(built_sources) $(built_headers)

nodist_libgstsubsink_la_SOURCES = $(built_sources)

CLEANFILES = $(BUILT_SOURCES)

EXTRA_DIST = gstsubsink-marshal.list

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
