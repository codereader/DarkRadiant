AM_CPPFLAGS = -I../../include -I../../libs $(GTK_CFLAGS)

pkglib_LTLIBRARIES = image.la
image_la_LDFLAGS = -module -avoid-version
image_la_LIBADD = ../../libs/jpeg6/libjpeg.a
image_la_SOURCES = dds.cpp \
                   image.cpp \
                   bmp.cpp \
                   jpeg.cpp \
                   ImageGDK.cpp \
                   pcx.cpp \
                   tga.cpp

# Override the Automake install rule, we want to install in $(pkglibdir)/modules
# not just $(pkglibdir).
install-exec-am:
	mkdir -p "$(DESTDIR)$(pkglibdir)/modules" && \
	$(INSTALL) .libs/image.so "$(DESTDIR)$(pkglibdir)/modules"

