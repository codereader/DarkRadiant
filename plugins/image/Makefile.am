AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/libs $(GTKMM_CFLAGS)

modulesdir = $(pkglibdir)/modules
modules_LTLIBRARIES = image.la

image_la_LDFLAGS = -module -avoid-version \
                   $(GTKMM_LIBS) \
                   $(JPEG_LIBS) \
                   $(GLEW_LIBS) \
                   $(GLU_LIBS) \
                   $(GL_LIBS) \
                   $(PNG_LIBS)
image_la_LIBADD = $(top_builddir)/libs/ddslib/libdds.la
image_la_SOURCES = dds.cpp \
                   image.cpp \
                   bmp.cpp \
                   jpeg.cpp \
                   DDSImage.cpp \
                   ImageGDK.cpp \
                   pcx.cpp \
                   tga.cpp \
                   png.cpp


