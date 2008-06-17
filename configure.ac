AC_INIT([darkradiant], [1.0])
AM_INIT_AUTOMAKE

# Setup and defaults
AC_LANG(C++)
AC_PROG_CXX
AC_DISABLE_STATIC
AC_PROG_LIBTOOL

# Check for required libraries

PKG_CHECK_MODULES([GTK], [gtk+-2.0], [], 
                  [AC_MSG_FAILURE([GTK libraries not found.])])
PKG_CHECK_MODULES([GTKGLEXT], [gtkglext-1.0])
PKG_CHECK_MODULES([XML], [libxml-2.0])

AC_CHECK_HEADER([GL/glew.h], [], [AC_MSG_ERROR([GLEW not found])])
AC_CHECK_LIB([GLEW], [main],
             [GLEW_LIBS='-lGLEW'],
             [AC_MSG_ERROR([GLEW not found])])
AC_SUBST([GLEW_LIBS])

AC_CHECK_LIB([GL], [main], 
             [GL_LIBS='-lGL'],
             [AC_MSG_ERROR([OpenGL libraries not found.])])
AC_SUBST([GL_LIBS])

# Generate output files
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile 
                 radiant/Makefile 
                 libs/Makefile
                 libs/scene/Makefile
                 libs/gtkutil/Makefile
                 libs/xmlutil/Makefile
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
                 plugins/model/Makefile])

AC_OUTPUT

