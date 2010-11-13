AM_CPPFLAGS = -I$(top_srcdir)/include \
			  -I$(top_srcdir)/libs \
			  -DPKGLIBDIR='"$(pkglibdir)"' \
			  -DPKGDATADIR='"$(pkgdatadir)"' \
			  $(GTKMM_CFLAGS) \
			  $(GTKGLEXTMM_CFLAGS) \
			  $(XML_CFLAGS) \
			  $(GTKSOURCEVIEWMM_CFLAGS)
AM_CXXFLAGS = -fPIC

pkglib_LTLIBRARIES = libgtkutil.la
libgtkutil_la_LDFLAGS = -release @PACKAGE_VERSION@ \
						$(GTKGLEXTMM_LIBS) $(GTKMM_LIBS) $(XML_LIBS) \
						$(GTKSOURCEVIEWMM_LIBS)
libgtkutil_la_LIBADD = $(top_builddir)/libs/xmlutil/libxmlutil.la
libgtkutil_la_SOURCES = ConsoleView.cpp \
						GLWidget.cpp \
                        GladeWidgetPopulator.cpp \
						PanedPosition.cpp \
						VFSTreePopulator.cpp \
						Timer.cpp \
						WindowPosition.cpp \
						window/PersistentTransientWindow.cpp \
                        window/TransientWindow.cpp \
						dialog/Dialog.cpp \
						dialog/MessageBox.cpp \
						dialog_legacy.cpp \
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
						clipboard.cpp \
						FileChooser.cpp
