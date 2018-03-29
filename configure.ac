AC_INIT([darkradiant], [2.5.0])
AM_INIT_AUTOMAKE([subdir-objects])
AM_SILENT_RULES([yes])

AC_CONFIG_MACRO_DIR([m4])

# Get info about the OS we're on
AC_CANONICAL_HOST

build_linux=no
build_osx=no

case $host_os in
  linux*)
        build_linux=yes
        ;;
  darwin* )
        build_osx=yes
        ;;
esac

# Initialise Gettext
AM_GNU_GETTEXT_VERSION(0.18.1)
AM_GNU_GETTEXT([external])

# greebo: Require that the gettext binaries are available
AC_CHECK_PROG(HAS_MSGFMT, msgfmt, [yes], [no])
AC_CHECK_PROG(HAS_MSGMERGE, msgmerge, [yes], [no])
AC_CHECK_PROG(HAS_XGETTEXT, xgettext, [yes], [no])

if test x"$HAS_MSGFMT" == x"no" || test x"$HAS_MSGMERGE" == x"no" || test x"$HAS_XGETTEXT" == x"no"
then
	AC_MSG_ERROR([Please make sure msgfmt, msgmerge and xgettext are available (probably through the gettext package).])
fi

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

# We require a C++11-compliant compiler, without nonstandard extensions
AX_CXX_COMPILE_STDCXX([11], [noext], [mandatory])

# We require the cstdint header to be present
AC_CHECK_HEADER([cstdint], [], [AC_MSG_ERROR([Could not find the <cstdint> header.])])

# Optional features
AC_ARG_ENABLE([debug],
              [AS_HELP_STRING([--enable-debug],
                              [enable debug build])],
              [debug_build=$enableval],
              [debug_build='no'])
AC_ARG_ENABLE([relocation],
              [AS_HELP_STRING([--enable-relocation],
                              [build a relocatable package without hard-coded
                               absolute paths])],
              [reloc_build=$enableval],
              [reloc_build='no'])
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

AC_ARG_WITH(pybind11,
            [--with-pybind11=/path/to/pybind11/include pybind11 include path to use (optional)],
            pybind11_include_path="$withval")

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

# wxWidgets
WX_CONFIG_OPTIONS
WX_CONFIG_CHECK([3.0.0], [wxWin=1], [wxWin=0], [std,gl,stc])
if test "$wxWin" != 1; then
	AC_MSG_ERROR([
                wxWidgets must be installed on your system
                but wx-config script could not be found.

                Please check that wx-config is in path, the directory
                where wxWidgets libraries are installed (returned by
                'wx-config --libs' command) is in LD_LIBRARY_PATH or
                equivalent variable and wxWidgets version is 2.3.4 or above.
        ])
fi

# Support libraries
PKG_CHECK_MODULES([XML], [libxml-2.0])
PKG_CHECK_MODULES([LIBSIGC], [sigc++-2.0])
PKG_CHECK_MODULES([PNG], [libpng])
PKG_CHECK_MODULES([FTGL], [ftgl])

# GLEW
AC_CHECK_HEADER([GL/glew.h], [], [AC_MSG_ERROR([GLEW not found])])
AC_CHECK_LIB([GLEW], [main],
             [GLEW_LIBS='-lGLEW'],
             [AC_MSG_ERROR([GLEW not found])])
AC_SUBST([GLEW_LIBS])

# In OSX, OpenGL and GLU are handled through the -framework OpenGL switch
# In Linux we need to check for the libs
if test "$build_osx" != "no"; then
    [GL_LIBS='-framework OpenGL']
    
    # We need the libintl library in OSX
    AC_CHECK_LIB([intl], [main],
                [INTL_LIBS='-lintl'],
                [AC_MSG_ERROR([intl library not found])])
    AC_SUBST([INTL_LIBS])
fi

if test "$build_linux" != "no"; then
    AC_CHECK_LIB([GL], [main],
             [GL_LIBS='-lGL'],
             [AC_MSG_ERROR([GL library not found.])])
	AC_CHECK_LIB([GLU], [gluBuild2DMipmaps],
             [GLU_LIBS="-lGLU"],
             [AC_MSG_ERROR([GLU library not found.])])
	AC_SUBST([GL_LIBS])
	AC_SUBST([GLU_LIBS])
fi

# By default, we rely on boost.filesystem, unless we have the C++17 libs 
use_boost_filesystem=yes

# Check for the regular C++17 <filesystem> header
AC_CHECK_HEADER([filesystem], [found_std_filesystem=yes], [found_std_filesystem=no])

# if no <filesystem> is present, check for the pre-C++17 header <experimental/filesystem>
if test "$found_std_filesystem" = "yes"
then
    AC_DEFINE([HAVE_STD_FILESYSTEM], [1], [Define this to indicate that the <filesystem> header is available for inclusion.])
    use_boost_filesystem=no
else
    AC_CHECK_HEADER([experimental/filesystem], [found_experimental_filesystem=yes], [found_experimental_filesystem=no])
    
    if test "$found_experimental_filesystem" = "yes"
    then
        AC_DEFINE([HAVE_EXPERIMENTAL_FILESYSTEM], [1], [Define this to indicate that the <experimental/filesystem> 
                                                        header is available for inclusion.])
        use_boost_filesystem=no
    fi
fi

FILESYSTEM_LIBS=''

# Require Boost Filesystem if the C++17-style header is not present
if test "$use_boost_filesystem" = "yes"
then
    AC_MSG_NOTICE([Will check for boost libraries since std::filesystem is not available])

    # Boost
    BOOST_REQUIRE([1.46.1])

    BOOST_FILESYSTEM
    BOOST_SYSTEM
    FILESYSTEM_LIBS="$(BOOST_FILESYSTEM_LIBS) $(BOOST_SYSTEM_LIBS)"
else
    AC_MSG_NOTICE([Will use std::filesystem instead of boost.filesystem])
    FILESYSTEM_LIBS="-lstdc++fs"
fi

AC_SUBST([FILESYSTEM_LIBS])

# PyBind11 if required
if test "$python_scripting" = 'yes'
then
    AM_CONDITIONAL([SYSTEM_HAS_PYBIND11], [false]) # default to "not found"
    
    AC_CHECK_PROGS([PYTHON_CONFIG], [python-config], [])
    if test ! -z "$PYTHON_CONFIG"
    then
        PYTHON_LIBS=`$PYTHON_CONFIG --libs`
        PYTHON_CPPFLAGS=`$PYTHON_CONFIG --includes`
        PYBIND11_CPPFLAGS=``
        
        # Add the path to the pybind11 library if we got one from the arguments
        if test "x$pybind11_include_path" != x ; then
           PYBIND11_CPPFLAGS="-I$pybind11_include_path"
        fi
        
        # pybind11.h is including Python.h, need to help it find the Python.h header
        CPPFLAGS="$USER_CPPFLAGS $PYTHON_CPPFLAGS $PYBIND11_CPPFLAGS"
        
        AC_MSG_NOTICE([Checking for pybind11 headers...])
        found_local_pybind11=no
        
        # Check for a system-provided pybind11, otherwise fall back on
        # the pybind11 headers we ship in the libs/pybind folder
        AC_CHECK_HEADER([pybind11/pybind11.h], [found_local_pybind11=yes], [found_local_pybind11=no])

        if test "$found_local_pybind11" = 'yes'
        then
                AC_MSG_NOTICE([Using the system-provided pybind11 headers])
            else
                AC_MSG_NOTICE([Using the pybind11 headers shipped with the sources])
        fi

        AM_CONDITIONAL([SYSTEM_HAS_PYBIND11], [test "$found_local_pybind11" = 'yes'])
        
        script_module='script'
        AC_SUBST([script_module])
        AC_SUBST([PYTHON_CPPFLAGS])
        AC_SUBST([PYTHON_LIBS])
        AC_SUBST([PYBIND11_CPPFLAGS])
        AC_SUBST([SYSTEM_HAS_PYBIND11]) # Used by Makefile.am in plugins/script/
    fi
else
    AM_CONDITIONAL([SYSTEM_HAS_PYBIND11], [false])
fi

# greebo: Don't check for the system-provided fmtlib, since linking
# against the libfmt.a produces a relocation linker error, complaining about
# a missing -fPIC switch. I won't bother with that right now, let's use the
# headers shipped with the repository.

#AC_MSG_NOTICE([Checking for fmtlib headers...])
found_local_fmtlib=no

# Check for a system-provided fmtlib, otherwise fall back on
# the headers we ship in the libs/libfmt folder
#AC_CHECK_HEADER([fmt/format.h], [found_local_fmtlib=yes], [found_local_fmtlib=no])

if test "$found_local_fmtlib" = 'yes'
then
    FMTLIB_CPPFLAGS="" # system got one
    FMTLIB_LDFLAGS="-lfmt"
    AC_MSG_NOTICE([Using the system-provided fmtlib headers])
else
    # Switch to header-only mode when using our own shipped sources
    FMTLIB_CPPFLAGS="-I`pwd`/libs/libfmt -DFMT_HEADER_ONLY"
    FMTLIB_LDFLAGS=""
    AC_MSG_NOTICE([Using the fmtlib headers shipped with the sources (header-only mode)])
fi

# dynamic link library
AC_CHECK_LIB([dl], [main],
             [DL_LIBS='-ldl'],
             [AC_MSG_ERROR([DL library not found])])
AC_SUBST([DL_LIBS])

# OpenAL and vorbis
AC_CHECK_HEADER([AL/alut.h],
                [sound_module='sound'],
                [sound_module=''])
AC_CHECK_LIB([vorbisfile], [ov_clear],
             [VORBIS_LIBS='-lvorbisfile'],
             [sound_module=''])

# Use the OpenAL framework in OSX
if test "$build_osx" != "no"; then
    [AL_LIBS='-framework OpenAL']
else
    AC_CHECK_LIB([openal], [alGetError],
             [AL_LIBS='-lopenal'],
             [sound_module=''])
fi

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

if test "$build_osx" != "no"; then
    WARNING_FLAGS="$WARNING_FLAGS -Wno-inconsistent-missing-override -Wno-potentially-evaluated-expression"
fi

CFLAGS="$USER_CFLAGS $WARNING_FLAGS $WX_CFLAGS"
CXXFLAGS="$USER_CXXFLAGS $WARNING_FLAGS $WX_CXXFLAGS_ONLY"
CPPFLAGS="$USER_CPPFLAGS -DPOSIX -fPIC $WX_CPPFLAGS $LIBSIGC_CFLAGS -DWXINTL_NO_GETTEXT_MACRO $FMTLIB_CPPFLAGS"
LDFLAGS="$USER_LDFLAGS $LIBSIGC_LIBS $FMTLIB_LDFLAGS"

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

# Relocation
if test "$reloc_build" != 'no'
then
    CPPFLAGS="-DENABLE_RELOCATION $CPPFLAGS"
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
                 libs/wxutil/Makefile
                 libs/math/Makefile
                 libs/scene/Makefile
                 libs/xmlutil/Makefile
                 plugins/Makefile
                 plugins/script/Makefile
                 plugins/sound/Makefile
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

echo     " Installation prefix:   $prefix"

if test "$reloc_build" != 'no'
then
    echo " Relocatable package:   yes"
else
    echo " Relocatable package:   no"
fi

if test -n "$sound_module"
then
    echo " Sound:                 yes"
else
    echo " Sound:                 no"
fi

if test "$darkmod_plugins" != 'no'
then
    echo " Darkmod plugins:       yes"
else
    echo " Darkmod plugins:       no"
fi

if test -z "$script_module"
then
    echo " Python scripting:      no"
else
    echo " Python scripting:      yes"
    
    if test "$found_local_pybind11" = "yes"
    then
        echo " Use system pybind11:   yes"
    else
        echo " Use system pybind11:   no"
    fi
fi

#if test "$found_local_fmtlib" = "yes"
#then
#    echo " Use system fmtlib:     yes"
#else
#    echo " Use system fmtlib:     no"
#fi

echo " Use boost.filesystem:  $use_boost_filesystem"
