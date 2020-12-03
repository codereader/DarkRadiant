cmake_minimum_required(VERSION 3.12)

# Project name and version
project(darkradiant VERSION 2.9.2)

# C++ standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Build shared libraries by default
option(BUILD_SHARED_LIBS "Build shared libraries" ON)

# Locate system packages
include(FindPkgConfig)
pkg_check_modules(XML libxml-2.0 REQUIRED)
pkg_check_modules(SIGC sigc++-2.0 REQUIRED)
pkg_check_modules(FTGL ftgl REQUIRED)

# Locate wxWidgets
find_package(wxWidgets REQUIRED COMPONENTS base)
message("Using wxWidgets file ${wxWidgets_USE_FILE}")
include(${wxWidgets_USE_FILE})

# Global includes and flags
include_directories(libs libs/libfmt include)
add_compile_definitions(POSIX WXINTL_NO_GETTEXT_MACRO)

# Supporting libraries
add_subdirectory(libs/math)
add_subdirectory(libs/xmlutil)
add_subdirectory(libs/scene)
add_subdirectory(libs/wxutil)
add_subdirectory(libs/module)

# Main radiant components
add_subdirectory(radiantcore)
