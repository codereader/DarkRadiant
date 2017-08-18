
# If system doesn't provide pybind11, fall back to the pybind11 headers we ship in the libs/pybind folder
if !SYSTEM_HAS_PYBIND11
PYBIND11_CPPFLAGS += -I$(top_srcdir)/libs/pybind
endif

AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/libs \
              $(XML_CFLAGS) \
              $(PYTHON_CPPFLAGS) \
              $(PYBIND11_CPPFLAGS) \
              -DPKGLIBDIR='"$(pkglibdir)"'

modulesdir = $(pkglibdir)/modules
modules_LTLIBRARIES = script.la

script_la_LDFLAGS = -module -avoid-version \
                    $(BOOST_SYSTEM_LIBS) \
                    $(BOOST_FILESYSTEM_LIBS) \
                    $(PYTHON_LIBS) \
                    $(WX_LIBS)
script_la_LIBADD = $(top_builddir)/libs/math/libmath.la \
                   $(top_builddir)/libs/wxutil/libwxutil.la
script_la_SOURCES = ScriptingSystem.cpp \
               ScriptCommand.cpp \
               ScriptModule.cpp \
               ScriptMenu.cpp \
               ScriptWindow.cpp \
               SceneNodeBuffer.cpp \
               PythonModule.cpp \
               interfaces/DialogInterface.cpp \
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
               interfaces/SceneGraphInterface.cpp \
               interfaces/ShaderSystemInterface.cpp \
               interfaces/SkinInterface.cpp \
               interfaces/SelectionSetInterface.cpp \
               interfaces/SelectionGroupInterface.cpp \
               interfaces/SoundInterface.cpp \
               interfaces/GameInterface.cpp
