AM_CPPFLAGS = -I../../include -I../../libs $(GTK_CFLAGS)

modulesdir = $(pkglibdir)/modules
modules_LTLIBRARIES = image.la

image_la_LDFLAGS = -module -avoid-version $(GTK_LIBS)
image_la_LIBADD = ../../libs/jpeg6/libjpeg.a ../../libs/ddslib/libdds.a
image_la_SOURCES = dds.cpp \
                   image.cpp \
                   bmp.cpp \
                   jpeg.cpp \
                   ImageGDK.cpp \
                   pcx.cpp \
                   tga.cpp

