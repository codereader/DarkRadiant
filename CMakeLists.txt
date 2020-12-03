cmake_minimum_required(VERSION 3.10)

# Project name and version
project(darkradiant VERSION 2.9.2)

# C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Global includes
include_directories(libs include)

# Subdirectories
add_subdirectory(libs/math)
