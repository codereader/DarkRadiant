AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/libs \
              $(XML_CFLAGS) $(PYTHON_CFLAGS) $(GTK_CFLAGS) \
              -DPKGLIBDIR='"$(pkglibdir)"'

modulesdir = $(pkglibdir)/modules
modules_LTLIBRARIES = script.la

script_la_LDFLAGS = -module -avoid-version \
					$(BOOST_FILESYSTEM_LIBS) \
                    $(BOOST_PYTHON_LIBS) \
                    $(PYTHON_LIBS) \
                    $(GTK_LIBS)
script_la_LIBADD = $(top_builddir)/libs/math/libmath.la \
                   $(top_builddir)/libs/gtkutil/libgtkutil.la
script_la_SOURCES = ScriptingSystem.cpp \
					ScriptCommand.cpp \
					ScriptModule.cpp \
					ScriptWindow.cpp \
					interfaces/EClassInterface.cpp \
					interfaces/BrushInterface.cpp \
					interfaces/RadiantInterface.cpp \
					interfaces/PatchInterface.cpp \
					interfaces/SelectionInterface.cpp \
					interfaces/MapInterface.cpp \
					interfaces/EntityInterface.cpp \
					interfaces/MathInterface.cpp \
					interfaces/ModelInterface.cpp \
					interfaces/CommandSystemInterface.cpp \
					interfaces/FileSystemInterface.cpp \
					interfaces/GridInterface.cpp \
					interfaces/ShaderSystemInterface.cpp \
					interfaces/SkinInterface.cpp \
					interfaces/SoundInterface.cpp \
					interfaces/GameInterface.cpp
