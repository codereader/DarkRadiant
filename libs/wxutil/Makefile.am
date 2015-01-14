AM_CPPFLAGS = -I$(top_srcdir)/include \
              -I$(top_srcdir)/libs \
              -DPKGLIBDIR='"$(pkglibdir)"' \
              -DPKGDATADIR='"$(pkgdatadir)"' \
              $(XML_CFLAGS) \
              $(LIBSIGC_CFLAGS) \
              $(FTGL_CFLAGS)
AM_CXXFLAGS = -fPIC

pkglib_LTLIBRARIES = libwxutil.la
libwxutil_la_LDFLAGS = -release @PACKAGE_VERSION@ \
                        $(XML_LIBS) \
                        $(GLEW_LIBS) \
                        $(LIBSIGC_LIBS) \
                        $(WX_LIBS) \
                        $(GL_LIBS) \
                        $(GLU_LIBS) \
                        $(FTGL_LIBS)
libwxutil_la_LIBADD = $(top_builddir)/libs/xmlutil/libxmlutil.la \
                      $(top_builddir)/libs/math/libmath.la \
                      $(top_builddir)/libs/scene/libscenegraph.la
libwxutil_la_SOURCES = ConsoleView.cpp \
                       FreezePointer.cpp \
                       GLWidget.cpp \
                       KeyValueTable.cpp \
                       PanedPosition.cpp \
                       VFSTreePopulator.cpp \
                       WindowPosition.cpp \
                       window/TransientWindow.cpp \
                       dialog/Dialog.cpp \
                       dialog/MessageBox.cpp \
                       preview/ModelPreview.cpp \
                       preview/ParticlePreview.cpp \
                       preview/RenderPreview.cpp \
                       GLFont.cpp \
                       menu/PopupMenu.cpp \
                       PathEntry.cpp \
                       TreeModel.cpp \
                       TreeView.cpp \
                       TreeModelFilter.cpp \
                       ModalProgressDialog.cpp \
                       SerialisableWidgets.cpp \
                       SourceView.cpp \
                       clipboard.cpp \
                       DirChooser.cpp \
                       FileChooser.cpp
