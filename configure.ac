AC_INIT([darkradiant], [1.0])
AM_INIT_AUTOMAKE

# Setup and defaults
AC_LANG(C++)
AC_PROG_CXX
AC_DISABLE_STATIC
AC_PROG_LIBTOOL

# Check for required libraries

# GTK etc
PKG_CHECK_MODULES([GLIB], [glib-2.0], [],
                  [AC_MSG_ERROR([GLIB not found])])
PKG_CHECK_MODULES([GTK], [gtk+-2.0], [], 
                  [AC_MSG_FAILURE([GTK libraries not found.])])
PKG_CHECK_MODULES([GTKGLEXT], [gtkglext-1.0])
PKG_CHECK_MODULES([XML], [libxml-2.0])

# GLEW
AC_CHECK_HEADER([GL/glew.h], [], [AC_MSG_ERROR([GLEW not found])])
AC_CHECK_LIB([GLEW], [main],
             [GLEW_LIBS='-lGLEW'],
             [AC_MSG_ERROR([GLEW not found])])
AC_SUBST([GLEW_LIBS])

# OpenGL
AC_CHECK_LIB([GL], [main], 
             [GL_LIBS='-lGL'],
             [AC_MSG_ERROR([OpenGL libraries not found.])])
AC_SUBST([GL_LIBS])

# Boost.Regex
AC_CHECK_LIB([boost_regex], [main],
             [REGEX_LIBS='-lboost_regex'],
             [AC_MSG_ERROR([Boost.Regex not found])])
AC_SUBST([REGEX_LIBS])

# Check for optional libraries
AC_CHECK_HEADER([AL/alut.h],
                [sound_module='sound'],
                [sound_module=''])
AC_SUBST([sound_module])

# Generate output files
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile 
                 radiant/Makefile 
                 libs/Makefile
                 libs/scene/Makefile
                 libs/ddslib/Makefile
                 libs/gtkutil/Makefile
                 libs/xmlutil/Makefile
                 libs/jpeg6/Makefile
                 libs/math/Makefile
                 plugins/Makefile
                 plugins/archivezip/Makefile
                 plugins/eclassmgr/Makefile
                 plugins/entity/Makefile
                 plugins/eventmanager/Makefile
                 plugins/filetypes/Makefile
                 plugins/filters/Makefile
                 plugins/grid/Makefile
                 plugins/image/Makefile
                 plugins/mapdoom3/Makefile
                 plugins/md5model/Makefile
                 plugins/model/Makefile
                 plugins/particles/Makefile
                 plugins/scenegraph/Makefile
                 plugins/shaders/Makefile
                 plugins/skins/Makefile
                 plugins/sound/Makefile
                 plugins/uimanager/Makefile
                 plugins/undo/Makefile
                 plugins/vfspk3/Makefile
                 plugins/xmlregistry/Makefile])

AC_OUTPUT

