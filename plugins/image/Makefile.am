AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/libs $(GTKMM_CFLAGS)

modulesdir = $(pkglibdir)/modules
modules_LTLIBRARIES = image.la

image_la_LDFLAGS = -module -avoid-version \
                   $(WX_LIBS) \
                   $(JPEG_LIBS) \
                   $(GLEW_LIBS) \
                   $(GLU_LIBS) \
                   $(GL_LIBS) \
                   $(PNG_LIBS)
image_la_LIBADD = $(top_builddir)/libs/ddslib/libdds.la \
                  $(top_builddir)/libs/xmlutil/libxmlutil.la
image_la_SOURCES = dds.cpp \
                   Doom3ImageLoader.cpp \
                   image.cpp \
                   ImageLoaderWx.cpp \
                   DDSImage.cpp \
                   TGALoader.cpp


