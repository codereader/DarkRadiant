AC_INIT([darkradiant], [1.8.0])
AM_INIT_AUTOMAKE([subdir-objects])
AM_SILENT_RULES([yes])

AC_CONFIG_MACRO_DIR([m4])

# Initialise Gettext
AM_GNU_GETTEXT([external])

# Save user flags
USER_CFLAGS=$CFLAGS
USER_CXXFLAGS=$CXXFLAGS
USER_CPPFLAGS=$CPPFLAGS
USER_LDFLAGS=$LDFLAGS

# Setup and defaults
AC_LANG(C++)
AC_PROG_CXX
AC_DISABLE_STATIC
AC_PROG_LIBTOOL

# Optional features
AC_ARG_ENABLE([debug],
              [AS_HELP_STRING([--enable-debug],
                              [enable debug build])],
              [debug_build=$enableval],
              [debug_build='no'])
AC_ARG_ENABLE([darkmod-plugins],
              [AS_HELP_STRING([--enable-darkmod-plugins],
                              [build Dark Mod plugins such as S/R editor])],
              [darkmod_plugins=$enableval],
              [darkmod_plugins='no'])
AC_ARG_ENABLE([python],
              [AS_HELP_STRING([--disable-python],
                              [disable Python scripting functionality])],
              [python_scripting=$enableval],
              [python_scripting='yes'])

if [[ "$darkmod_plugins" != 'no' ]]
then
    # Enable conditional directories (used by plugins/Makefile.am)
    sr_plugin="dm.stimresponse"
    obj_plugin="dm.objectives"
    difficulty_plugin="dm.difficulty"
    conv_plugin="dm.conversation"
    gui_plugin="dm.gui"
    editing_plugin="dm.editing"
fi
AC_SUBST([sr_plugin])
AC_SUBST([obj_plugin])
AC_SUBST([difficulty_plugin])
AC_SUBST([conv_plugin])
AC_SUBST([gui_plugin])
AC_SUBST([editing_plugin])

# Check for required libraries

# Zlib
AC_CHECK_HEADER([zlib.h], [], [AC_MSG_ERROR([ZLib not found])])
AC_CHECK_LIB([z], [inflateEnd],
             [Z_LIBS='-lz'],
             [AC_MSG_ERROR([ZLib not found])])
AC_SUBST([Z_LIBS])

# JPEG
AC_CHECK_HEADER([jpeglib.h], [], [AC_MSG_ERROR([libjpeg header not found])])
AC_CHECK_LIB([jpeg], [jpeg_start_compress],
             [JPEG_LIBS='-ljpeg'],
             [AC_MSG_ERROR([libjpeg library not found])])
AC_SUBST([JPEG_LIBS])

# GTK etc
PKG_CHECK_MODULES([GLIB], [glib-2.0], [],
                  [AC_MSG_ERROR([GLIB not found])])
PKG_CHECK_MODULES([GTK], [gtk+-2.0], [],
                  [AC_MSG_FAILURE([GTK libraries not found.])])
PKG_CHECK_MODULES([GTKMM], [gtkmm-2.4], [],
                  [AC_MSG_FAILURE([GTKMM libraries not found])])
PKG_CHECK_MODULES([GTKGLEXT], [gtkglext-1.0])
PKG_CHECK_MODULES([GTKGLEXTMM], [gtkglextmm-1.2])
PKG_CHECK_MODULES([XML], [libxml-2.0])
PKG_CHECK_MODULES([LIBSIGC], [sigc++-2.0])
PKG_CHECK_MODULES([PNG], [libpng])
PKG_CHECK_MODULES([FTGL], [ftgl])

# GTKSourceView
PKG_CHECK_MODULES([GTKSOURCEVIEWMM],
                  [gtksourceviewmm-2.0],
                  [AC_DEFINE([HAVE_GTKSOURCEVIEW],
                             [],
                             [Define if GtkSourceView is available])],
                  [true])

# GLEW
AC_CHECK_HEADER([GL/glew.h], [], [AC_MSG_ERROR([GLEW not found])])
AC_CHECK_LIB([GLEW], [main],
             [GLEW_LIBS='-lGLEW'],
             [AC_MSG_ERROR([GLEW not found])])
AC_SUBST([GLEW_LIBS])

# OpenGL and GLU
AC_CHECK_LIB([GL], [main],
             [GL_LIBS='-lGL'],
             [AC_MSG_ERROR([GL library not found.])])
AC_CHECK_LIB([GLU], [gluBuild2DMipmaps],
             [GLU_LIBS="-lGLU"],
             [AC_MSG_ERROR([GLU library not found.])])
AC_SUBST([GL_LIBS])
AC_SUBST([GLU_LIBS])

# Boost
BOOST_REQUIRE([1.46.1])

# Check a load of common headers we use
BOOST_BIND
BOOST_FOREACH
BOOST_FORMAT
BOOST_FUNCTION
BOOST_SMART_PTR
BOOST_TOKENIZER

# These ones require libraries too
BOOST_REGEX
BOOST_FILESYSTEM
BOOST_SYSTEM
BOOST_TEST

# Boost.Python if required
if test "$python_scripting" = 'yes'
then
    AC_CHECK_PROGS([PYTHON_CONFIG], [python2-config], [])
    if test ! -z "$PYTHON_CONFIG"
    then
        PYTHON_LIBS=`$PYTHON_CONFIG --libs`

        BOOST_PYTHON
        script_module='script'
        AC_SUBST([script_module])
        AC_SUBST([PYTHON_LIBS])
    fi
fi

# dynamic link library
AC_CHECK_LIB([dl], [main],
             [DL_LIBS='-ldl'],
             [AC_MSG_ERROR([DL library not found])])
AC_SUBST([DL_LIBS])

# Dependent Boost headers
AC_CHECK_HEADER([boost/serialization/access.hpp], [],
                [AC_MSG_ERROR([Boost.Serialization headers not found])])

# OpenAL and vorbis
AC_CHECK_HEADER([AL/alut.h],
                [sound_module='sound'],
                [sound_module=''])
AC_CHECK_LIB([vorbisfile], [ov_clear],
             [VORBIS_LIBS='-lvorbisfile'],
             [sound_module=''])
AC_CHECK_LIB([openal], [alGetError],
             [AL_LIBS='-lopenal'],
             [sound_module=''])
AC_CHECK_LIB([alut], [main],
             [ALUT_LIBS='-lalut'],
             [sound_module=''],
             [$AL_LIBS])

AC_SUBST([sound_module])
AC_SUBST([ALUT_LIBS])
AC_SUBST([VORBIS_LIBS])
AC_SUBST([AL_LIBS])

# Configure global flags, cancelling any modifications we may have made during
# configuration
WARNING_FLAGS="-Wall -Wno-unused-variable -Werror=return-type"
CFLAGS="$USER_CFLAGS $WARNING_FLAGS"
CXXFLAGS="$USER_CXXFLAGS $WARNING_FLAGS"
CPPFLAGS="$USER_CPPFLAGS -DPOSIX -std=c++0x"
LDFLAGS="$USER_LDFLAGS -Wl,-z,defs"

# Debug/optimisation
if test "$debug_build" != 'no'
then
    CPPFLAGS="-D_DEBUG $CPPFLAGS"
    CFLAGS="-g -O0 $CFLAGS"
    CXXFLAGS="-g -O0 $CXXFLAGS"
else
    CFLAGS="-g -O2 -DNDEBUG $CFLAGS"
    CXXFLAGS="-g -O2 -DNDEBUG $CXXFLAGS"
fi

AC_SUBST([CPPFLAGS])
AC_SUBST([CFLAGS])
AC_SUBST([CXXFLAGS])
AC_SUBST([LDFLAGS])

# Generate output files
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([install/darkradiant.desktop install/i18n//Makefile.in
                 Makefile
                 radiant/Makefile
                 install/i18n/Makefile.in
                 libs/Makefile
                 libs/ddslib/Makefile
                 libs/gtkutil/Makefile
                 libs/math/Makefile
                 libs/picomodel/Makefile
                 libs/scene/Makefile
                 libs/xmlutil/Makefile
                 plugins/Makefile
                 plugins/archivezip/Makefile
                 plugins/commandsystem/Makefile
                 plugins/eclassmgr/Makefile
                 plugins/eclasstree/Makefile
                 plugins/entity/Makefile
                 plugins/entitylist/Makefile
                 plugins/eventmanager/Makefile
                 plugins/filetypes/Makefile
                 plugins/filters/Makefile
                 plugins/fonts/Makefile
                 plugins/grid/Makefile
                 plugins/image/Makefile
                 plugins/mapdoom3/Makefile
                 plugins/md5model/Makefile
                 plugins/model/Makefile
                 plugins/particles/Makefile
                 plugins/scenegraph/Makefile
                 plugins/script/Makefile
                 plugins/shaders/Makefile
                 plugins/skins/Makefile
                 plugins/sound/Makefile
                 plugins/uimanager/Makefile
                 plugins/undo/Makefile
                 plugins/vfspk3/Makefile
                 plugins/wavefront/Makefile
                 plugins/xmlregistry/Makefile
                 plugins/dm.stimresponse/Makefile
                 plugins/dm.objectives/Makefile
                 plugins/dm.difficulty/Makefile
                 plugins/dm.gui/Makefile
                 plugins/dm.editing/Makefile
                 plugins/dm.conversation/Makefile])

AC_OUTPUT

# Display configured options
echo "
Configuration options:"

echo     " Installation prefix: $prefix"
if test -n "$sound_module"
then
    echo " Sound:               yes"
else
    echo " Sound:               no"
fi

if test "$darkmod_plugins" != 'no'
then
    echo " Darkmod plugins:     yes"
else
    echo " Darkmod plugins:     no"
fi

if test -z "$script_module"
then
    echo " Python scripting:    no"
else
    echo " Python scripting:    yes"
fi

