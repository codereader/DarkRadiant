AC_INIT([darkradiant], [0.9.9])
AM_INIT_AUTOMAKE

# Save user flags
USER_CFLAGS=$CFLAGS
USER_CXXFLAGS=$CXXFLAGS

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

if [[ $darkmod_plugins != 'no' ]]
then
    # Enable conditional directories (used by plugins/Makefile.am)
    sr_plugin="dm.stimresponse"
    obj_plugin="dm.objectives"
    difficulty_plugin="dm.difficulty"
    conv_plugin="dm.conversation"
fi
AC_SUBST([sr_plugin])
AC_SUBST([obj_plugin])
AC_SUBST([difficulty_plugin])
AC_SUBST([conv_plugin])

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
PKG_CHECK_MODULES([GTKGLEXT], [gtkglext-1.0])
PKG_CHECK_MODULES([XML], [libxml-2.0])

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

# Boost.Regex
AC_CHECK_LIB([boost_regex], [main],
             [REGEX_LIBS='-lboost_regex'],
             [AC_MSG_ERROR([Boost.Regex not found])])
AC_SUBST([REGEX_LIBS])

# Boost.Filesystem
AC_CHECK_LIB([boost_filesystem], [main],
             [BOOST_FILESYSTEM_LIBS='-lboost_filesystem'],
             [AC_MSG_ERROR([Boost.Filesystem not found])])
AC_SUBST([BOOST_FILESYSTEM_LIBS])

# Dependent Boost headers
AC_CHECK_HEADER([boost/serialization/access.hpp], [],
                [AC_MSG_ERROR([Boost.Serialization headers not found])])

# OpenAL and vorbis
AC_CHECK_HEADER([AL/alut.h],
                [sound_module='sound'],
                [sound_module=''])
AC_CHECK_LIB([alut], [main],
             [ALUT_LIBS='-lalut'],
             [sound_module=''])
AC_CHECK_LIB([vorbisfile], [ov_clear],
             [VORBIS_LIBS='-lvorbisfile'],
             [sound_module=''])
AC_CHECK_LIB([openal], [alGetError],
             [AL_LIBS='-lopenal'],
             [sound_module=''])

AC_SUBST([sound_module])
AC_SUBST([ALUT_LIBS])
AC_SUBST([VORBIS_LIBS])
AC_SUBST([AL_LIBS])

# Configure global flags
CFLAGS="$USER_CFLAGS -DPOSIX"
CXXFLAGS="$USER_CXXFLAGS -DPOSIX"

# Debug/optimisation
if [[ $debug_build != 'no' ]]
then
    CFLAGS="-g -O0 $CFLAGS"
    CXXFLAGS="-g -O0 $CXXFLAGS"
else
    CFLAGS="-g -O2 $CFLAGS"
    CXXFLAGS="-g -O2 $CXXFLAGS"
fi

AC_SUBST([CFLAGS])
AC_SUBST([CXXFLAGS])

# Generate output files
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile 
                 radiant/Makefile
                 libs/Makefile
                 libs/ddslib/Makefile
                 libs/gtkutil/Makefile
                 libs/jpeg6/Makefile
                 libs/math/Makefile
                 libs/picomodel/Makefile
                 libs/scene/Makefile
                 libs/xmlutil/Makefile
                 plugins/Makefile
                 plugins/archivezip/Makefile
                 plugins/eclassmgr/Makefile
                 plugins/eclasstree/Makefile
                 plugins/entity/Makefile
                 plugins/entitylist/Makefile
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
                 plugins/xmlregistry/Makefile
                 plugins/dm.stimresponse/Makefile
                 plugins/dm.objectives/Makefile
                 plugins/dm.difficulty/Makefile
                 plugins/dm.conversation/Makefile])

AC_OUTPUT

# Display configured options
echo "
Configuration options:"

echo     " Installation prefix: $prefix"
if [[ $sound_module != '' ]]
then
    echo " Sound:               yes"
else
    echo " Sound:               no"
fi

if [[ $darkmod_plugins != 'no' ]]
then
    echo " Darkmod plugins:     yes"
else
    echo " Darkmod plugins:     no"
fi

