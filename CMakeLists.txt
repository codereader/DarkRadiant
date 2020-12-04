cmake_minimum_required(VERSION 3.12)

# Project name and version
project(darkradiant VERSION 2.9.2)

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Build shared libraries by default
option(BUILD_SHARED_LIBS "Build shared libraries" ON)

# Define GNU-style directory structure by default, and pass directories to the
# code with defines
include(GNUInstallDirs)
add_compile_definitions(LOCALEDIR="${CMAKE_INSTALL_FULL_LOCALEDIR}")

# Locate system packages
include(FindPkgConfig)
pkg_check_modules(XML libxml-2.0 REQUIRED)
pkg_check_modules(SIGC sigc++-2.0 REQUIRED)
pkg_check_modules(FTGL ftgl REQUIRED)
pkg_check_modules(FREETYPE freetype2 REQUIRED)
pkg_check_modules(XML libxml-2.0 REQUIRED)
pkg_check_modules(GL gl REQUIRED)
pkg_check_modules(GLEW glew REQUIRED)
pkg_check_modules(JPEG libjpeg REQUIRED)
pkg_check_modules(PNG libpng REQUIRED)

# Locate wxWidgets
find_package(wxWidgets REQUIRED
             COMPONENTS base core stc adv gl xrc)
message("Using wxWidgets file ${wxWidgets_USE_FILE}")
include(${wxWidgets_USE_FILE})

# Global includes and flags
include_directories(libs libs/libfmt include)
add_compile_definitions(POSIX
                        WXINTL_NO_GETTEXT_MACRO
                        FMT_HEADER_ONLY
                        HAVE_STD_FILESYSTEM)
add_link_options(LINKER:-z,defs)

# Supporting libraries
add_subdirectory(libs/math)
add_subdirectory(libs/xmlutil)
add_subdirectory(libs/scene)
add_subdirectory(libs/wxutil)
add_subdirectory(libs/module)

# Main radiant components
add_subdirectory(radiantcore)
add_subdirectory(radiant)
