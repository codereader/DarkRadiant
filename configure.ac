AC_INIT([darkradiant], [1.0])
AM_INIT_AUTOMAKE

AC_LANG(C++)
AC_PROG_CXX
AC_PROG_RANLIB

# Check for required libraries

PKG_CHECK_MODULES([GTK], [gtk+-2.0], [], 
                  [AC_MSG_FAILURE([GTK libraries not found.])])
PKG_CHECK_MODULES([GTKGLEXT], [gtkglext-1.0])
PKG_CHECK_MODULES([XML], [libxml-2.0])

AC_CHECK_LIB([GL], [main], 
             [GL_LIBS='-lGL'],
             [AC_MSG_FAILURE([OpenGL libraries not found.])])
AC_SUBST([GL_LIBS])

# Generate output files
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile 
                 radiant/Makefile 
                 libs/Makefile
                 libs/scene/Makefile
                 libs/gtkutil/Makefile
                 libs/xmlutil/Makefile
                 libs/math/Makefile])

AC_OUTPUT

