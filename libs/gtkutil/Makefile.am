AM_CPPFLAGS = -I$(top_srcdir)/include \
			  -I$(top_srcdir)/libs \
			  -DPKGLIBDIR='"$(pkglibdir)"' \
			  -DPKGDATADIR='"$(pkgdatadir)"' \
			  $(GTK_CFLAGS) \
			  $(GTKGLEXT_CFLAGS) \
			  $(XML_CFLAGS) \
			  $(GTKSOURCEVIEW_CFLAGS)
AM_CXXFLAGS = -fPIC

pkglib_LTLIBRARIES = libgtkutil.la
libgtkutil_la_LDFLAGS = -release @PACKAGE_VERSION@ \
						$(GTKGLEXT_LIBS) $(GTK_LIBS) $(XML_LIBS) \
						$(GTKSOURCEVIEW_LIBS)
libgtkutil_la_LIBADD = $(top_builddir)/libs/xmlutil/libxmlutil.la
libgtkutil_la_SOURCES = ConsoleView.cpp \
						GLWidget.cpp \
						PanedPosition.cpp \
						VFSTreePopulator.cpp \
						messagebox.cpp \
						nonmodal.cpp \
						Timer.cpp \
						WindowPosition.cpp \
						idledraw.cpp \
						window/PersistentTransientWindow.cpp \
						dialog.cpp \
						glfont.cpp \
						MenuItemAccelerator.cpp \
						cursor.cpp \
						DeferredAdjustment.cpp \
						menu/PopupMenu.cpp \
						widget.cpp \
						RegistryConnector.cpp \
						PathEntry.cpp \
						TreeModel.cpp \
						ModalProgressDialog.cpp \
						SerialisableWidgets.cpp \
						SourceView.cpp \
						window.cpp \
						clipboard.cpp \
						FileChooser.cpp \
						pointer.cpp
