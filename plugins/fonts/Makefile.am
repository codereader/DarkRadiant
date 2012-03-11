AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/libs \
              $(XML_CFLAGS) $(LIBSIGC_CFLAGS)

modulesdir = $(pkglibdir)/modules
modules_LTLIBRARIES = fonts.la

fonts_la_LIBADD = $(top_builddir)/libs/xmlutil/libxmlutil.la
fonts_la_LDFLAGS = -module -avoid-version \
                   $(XML_LIBS) \
                   $(GL_LIBS) \
                   $(GLU_LIBS) \
                   $(BOOST_REGEX_LIBS) \
                   $(LIBSIGC_LIBS)
fonts_la_SOURCES = plugin.cpp \
					FontLoader.cpp \
					GlyphInfo.cpp \
					GlyphSet.cpp \
					FontManager.cpp
