plugin_LTLIBRARIES = libgstvideofiltersbad.la

#ORC_SOURCE=gstvideofiltersbadorc
#include $(top_srcdir)/common/orc.mak

libgstvideofiltersbad_la_SOURCES = \
	gstzebrastripe.c \
	gstvideofiltersbad.c
#nodist_libgstvideofiltersbad_la_SOURCES = $(ORC_NODIST_SOURCES)
libgstvideofiltersbad_la_CFLAGS = \
	$(GST_PLUGINS_BASE_CFLAGS) \
	$(GST_CFLAGS) \
	$(ORC_CFLAGS)
libgstvideofiltersbad_la_LIBADD = \
	$(GST_PLUGINS_BASE_LIBS) -lgstvideo-$(GST_MAJORMINOR) \
	$(GST_BASE_LIBS) \
	$(GST_LIBS) \
	$(ORC_LIBS) \
	$(LIBM)
libgstvideofiltersbad_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
libgstvideofiltersbad_la_LIBTOOLFLAGS = --tag=disable-static

noinst_HEADERS = \
	gstzebrastripe.h


