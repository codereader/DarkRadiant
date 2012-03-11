AM_CPPFLAGS = -I$(top_srcdir)/include \
			  -I$(top_srcdir)/libs \
			  -DPKGLIBDIR='"$(pkglibdir)"' \
			  -DPKGDATADIR='"$(pkgdatadir)"' \
			  $(GTKMM_CFLAGS) \
			  $(GTKGLEXTMM_CFLAGS) \
			  $(XML_CFLAGS) \
			  $(GTKSOURCEVIEWMM_CFLAGS) \
              $(FTGL_CFLAGS)
AM_CXXFLAGS = -fPIC

pkglib_LTLIBRARIES = libgtkutil.la
libgtkutil_la_LDFLAGS = -release @PACKAGE_VERSION@ \
						$(GTKGLEXTMM_LIBS) \
						$(GTKMM_LIBS) \
						$(XML_LIBS) \
						$(GLEW_LIBS) \
						$(GTKSOURCEVIEWMM_LIBS) \
                        $(FTGL_LIBS)
libgtkutil_la_LIBADD = $(top_builddir)/libs/xmlutil/libxmlutil.la \
			$(top_builddir)/libs/math/libmath.la
libgtkutil_la_SOURCES = ConsoleView.cpp \
						FreezePointer.cpp \
						GLWidget.cpp \
                        KeyValueTable.cpp \
						PanedPosition.cpp \
						VFSTreePopulator.cpp \
						Timer.cpp \
						WindowPosition.cpp \
						window/PersistentTransientWindow.cpp \
                        window/TransientWindow.cpp \
						dialog/Dialog.cpp \
						dialog/MessageBox.cpp \
                        preview/ModelPreview.cpp \
                        preview/ParticlePreview.cpp \
						preview/RenderPreview.cpp \
						GLFont.cpp \
						MenuItemAccelerator.cpp \
						Cursor.cpp \
						menu/PopupMenu.cpp \
						PathEntry.cpp \
						TreeModel.cpp \
						ModalProgressDialog.cpp \
						SerialisableWidgets.cpp \
						SourceView.cpp \
						clipboard.cpp \
						FileChooser.cpp
