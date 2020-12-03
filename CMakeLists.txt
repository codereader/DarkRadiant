cmake_minimum_required(VERSION 3.10)

# Project name and version
project(darkradiant VERSION 2.9.2)

# C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Build shared libraries by default
option(BUILD_SHARED_LIBS "Build shared libraries" ON)

# Locate system packages
include(FindPkgConfig)
pkg_check_modules(XML libxml-2.0 REQUIRED)
pkg_check_modules(SIGC sigc++-2.0 REQUIRED)

# Global includes and flags
include_directories(libs libs/libfmt include)

# Subdirectories
add_subdirectory(libs/math)
add_subdirectory(libs/xmlutil)
add_subdirectory(libs/scene)
