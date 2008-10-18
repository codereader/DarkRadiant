AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/libs $(GTK_CFLAGS)

modulesdir = $(pkglibdir)/modules
modules_LTLIBRARIES = image.la

image_la_LDFLAGS = -module -avoid-version $(GTK_LIBS)
image_la_LIBADD = $(top_builddir)/libs/jpeg6/libjpeg.la \
				  $(top_builddir)/libs/ddslib/libdds.la
image_la_SOURCES = dds.cpp \
                   image.cpp \
                   bmp.cpp \
                   jpeg.cpp \
                   ImageGDK.cpp \
                   pcx.cpp \
                   tga.cpp

