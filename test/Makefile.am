AM_CPPFLAGS = -DTESTRESOURCEDIR='"$(abs_top_builddir)/test/resources"' \
              -I$(top_srcdir)/include -I$(top_srcdir)/libs \
               $(XML_CFLAGS)

check_PROGRAMS = drtest
TESTS = $(check_PROGRAMS)

drtestdir = $(pkglibdir)/bin/
drtest_CPPFLAGS = $(AM_CPPFLAGS) 
drtest_LDFLAGS = -lpthread -lgtest -lgtest_main -lX11 \
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
                $(Z_LIBS)
drtest_LDADD = $(top_builddir)/libs/scene/libscenegraph.la \
            $(top_builddir)/libs/xmlutil/libxmlutil.la \
            $(top_builddir)/libs/math/libmath.la \
            $(top_builddir)/libs/module/libmodule.la
drtest_SOURCES = math/Matrix4.cpp \
                 math/Vector3.cpp \
                 math/Plane3.cpp \
                 math/Quaternion.cpp \
                 Camera.cpp \
                 ColourSchemes.cpp \
                 CSG.cpp \
                 HeadlessOpenGLContext.cpp \
                 FacePlane.cpp \
                 FileTypes.cpp \
                 MapSavingLoading.cpp \
                 Materials.cpp \
                 MapExport.cpp \
                 MessageBus.cpp \
                 Models.cpp \
                 ModelExport.cpp \
                 ModelScale.cpp \
                 PatchIterators.cpp \
                 PatchWelding.cpp \
                 Selection.cpp \
                 SelectionAlgorithm.cpp \
                 VFS.cpp