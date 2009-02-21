AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/libs \
              $(XML_CFLAGS) $(PYTHON_CFLAGS) \
              -DPKGLIBDIR='"$(pkglibdir)"'

modulesdir = $(pkglibdir)/modules
modules_LTLIBRARIES = script.la

script_la_LDFLAGS = -module -avoid-version $(BOOST_PYTHON_LIBS) $(PYTHON_LIBS)
script_la_SOURCES = ScriptingSystem.cpp \
					ScriptModule.cpp \
					interfaces/EClassInterface.cpp \
					interfaces/BrushInterface.cpp \
					interfaces/RadiantInterface.cpp \
					interfaces/PatchInterface.cpp \
					interfaces/SelectionInterface.cpp \
					interfaces/MapInterface.cpp \
					interfaces/EntityInterface.cpp \
					interfaces/MathInterface.cpp
