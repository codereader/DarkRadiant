#pluginsdir = $(pkglibdir)/plugins
core_LTLIBRARIES = core.la

core_CPPFLAGS = -DPKGLIBDIR='"$(pkglibdir)"' \
              -DPKGDATADIR='"$(pkgdatadir)"' \
              -DLOCALEDIR='"$(localedir)"' \
              -DHTMLDIR='"$(htmldir)"' \
              -I$(top_srcdir)/include \
              -I$(top_srcdir)/libs \
              $(LIBSIGC_CFLAGS) \
              $(XML_CFLAGS) \
              $(FTGL_CFLAGS)

core_LDADD = $(top_builddir)/libs/scene/libscenegraph.la \
             $(top_builddir)/libs/wxutil/libwxutil.la \
             $(top_builddir)/libs/xmlutil/libxmlutil.la \
             $(top_builddir)/libs/math/libmath.la

core_la_LDFLAGS = -module -avoid-version \
                    $(XML_LIBS) \
                    $(GLEW_LIBS) \
                    $(GL_LIBS) \
                    $(GLU_LIBS) \
                    $(JPEG_LIBS) \
                    $(PNG_LIBS) \
                    $(FTGL_LIBS) \
                    $(LIBSIGC_LIBS) \
                    $(FILESYSTEM_LIBS) \
                    $(DL_LIBS) \
                    $(INTL_LIBS) \
                    $(WX_LIBS) \
                    $(Z_LIBS)

core_la_SOURCES = Radiant.cpp \
                commandsystem/CommandSystem.cpp \
                log/COutRedirector.cpp \
                log/LogFile.cpp \
                log/LogStream.cpp \
                log/LogStreamBuf.cpp \
                log/LogWriter.cpp \
                log/StringLogDevice.cpp \
                modulesystem/ModuleLoader.cpp \
                modulesystem/ModuleRegistry.cpp \
                settings/Game.cpp \
                settings/GameManager.cpp \
                settings/LanguageManager.cpp \
                settings/PreferencePage.cpp \
                settings/PreferenceSystem.cpp \
                vfs/DeflatedInputStream.cpp \
                vfs/DirectoryArchive.cpp \
                vfs/Doom3FileSystem.cpp \
                vfs/Doom3FileSystemModule.cpp \
                vfs/ZipArchive.cpp \
                xmlregistry/RegistryTree.cpp \
                xmlregistry/XMLRegistry.cpp

