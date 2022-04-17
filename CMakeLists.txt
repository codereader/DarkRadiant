cmake_minimum_required(VERSION 3.12)

# Project name and version
project(darkradiant VERSION 3.0.0)

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# GCC 8 and earlier require explicit linking against stdc++fs
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9)
	link_libraries(stdc++fs)
endif()

# Expose some build options
option(ENABLE_DM_PLUGINS "Build Dark Mod specific plugins" ON)
option(ENABLE_RELOCATION
       "Avoid hard-coded absolute paths to libraries or resources"
       ON)

# Define GNU-style directory structure by default
include(GNUInstallDirs)

# Set up core build paths
set(CORE_MODULE_LIBRARY "libradiantcore")
set(PKGDATADIR "${CMAKE_INSTALL_FULL_DATADIR}/darkradiant")
set(PKGLIBDIR "${CMAKE_INSTALL_FULL_LIBDIR}/darkradiant")
if (${ENABLE_RELOCATION})
    set(RELATIVE_LIBDIR "../${CMAKE_INSTALL_LIBDIR}/darkradiant")
else()
    set(HTMLDIR "${CMAKE_INSTALL_FULL_DATADIR}/doc/darkradiant")
endif()

# Build shared libraries by default
option(BUILD_SHARED_LIBS "Build shared libraries" ON)
if (${ENABLE_RELOCATION})
    set(CMAKE_INSTALL_RPATH "$ORIGIN/${RELATIVE_LIBDIR}")
else()
    set(CMAKE_INSTALL_RPATH "${PKGLIBDIR}")
endif()

# Debug or release mode
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
add_compile_definitions(
    $<$<CONFIG:Debug>:_DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
)

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
pkg_check_modules(AL openal REQUIRED)
pkg_check_modules(OGG ogg REQUIRED)
pkg_check_modules(VORBIS vorbisfile REQUIRED)
pkg_check_modules(X11 x11 REQUIRED)
pkg_check_modules(ZLIB zlib REQUIRED)
pkg_check_modules(GLIB glib-2.0 REQUIRED)
pkg_check_modules(EIGEN eigen3 REQUIRED)

# Locate wxWidgets
find_package(wxWidgets REQUIRED
             COMPONENTS base core stc adv gl xrc aui)
include(${wxWidgets_USE_FILE})

# Locate Python
find_package(Python REQUIRED COMPONENTS Development)

# Global includes and flags
include_directories(libs libs/libfmt include)
add_compile_definitions(POSIX
                        WXINTL_NO_GETTEXT_MACRO
                        FMT_HEADER_ONLY
                        HAVE_STD_FILESYSTEM)
add_link_options(LINKER:-z,defs)

# Generate config.h
configure_file(config.h.in config.h)
add_compile_definitions(HAVE_CONFIG_H)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# Supporting libraries
add_subdirectory(libs/math)
add_subdirectory(libs/xmlutil)
add_subdirectory(libs/scene)
add_subdirectory(libs/wxutil)
add_subdirectory(libs/module)

# Mandatory modules
add_subdirectory(plugins/script)
add_subdirectory(plugins/sound)

pkg_check_modules(LIBGIT libgit2)
if (${LIBGIT_FOUND})
    add_subdirectory(plugins/vcs)
endif()

# Dark Mod plugins
if (${ENABLE_DM_PLUGINS})
    add_subdirectory(plugins/dm.conversation)
    add_subdirectory(plugins/dm.stimresponse)
    add_subdirectory(plugins/dm.objectives)
    add_subdirectory(plugins/dm.difficulty)
    add_subdirectory(plugins/dm.editing)
    add_subdirectory(plugins/dm.gui)
    add_subdirectory(plugins/dm.gameconnection)
endif()

# Main radiant components
add_subdirectory(radiantcore)
add_subdirectory(radiant)

# Tests
pkg_check_modules(GTEST gtest)
pkg_check_modules(GTEST_MAIN gtest_main)
if (${GTEST_FOUND} AND ${GTEST_MAIN_FOUND})
    include(CTest)
    add_subdirectory(test)
endif()

# Documentation
find_program(ASCIIDOCTOR asciidoctor)
if (ASCIIDOCTOR)
    add_subdirectory(doc)
else()
    message(WARNING "asciidoctor not found; documentation will not be generated")
endif()

# Install main targets
install(TARGETS darkradiant math xmlutil scenegraph wxutil
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION ${PKGLIBDIR})
install(TARGETS radiantcore script sound
        LIBRARY DESTINATION ${PKGLIBDIR}/modules)

# Install Dark Mod plugins
if (${ENABLE_DM_PLUGINS})
    install(TARGETS dm_stimresponse dm_objectives dm_difficulty dm_editing
                    dm_gui dm_gameconnection dm_conversation
            LIBRARY DESTINATION ${PKGLIBDIR}/plugins)
endif()

if (${LIBGIT_FOUND})
    install(TARGETS vcs
            LIBRARY DESTINATION ${PKGLIBDIR}/plugins)
endif()

# Generate and install the .desktop file
configure_file(install/darkradiant.desktop.in install/darkradiant.desktop)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/install/darkradiant.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)

# Install resources
file(GLOB XML_FILES "${PROJECT_SOURCE_DIR}/install/*.xml")
install(FILES ${XML_FILES} DESTINATION ${PKGDATADIR})

install(DIRECTORY install/games DESTINATION ${PKGDATADIR})
install(DIRECTORY install/bitmaps DESTINATION ${PKGDATADIR})
install(DIRECTORY install/gl DESTINATION ${PKGDATADIR})
install(DIRECTORY install/ui DESTINATION ${PKGDATADIR}
        FILES_MATCHING PATTERN "*.ttf" PATTERN "*.xrc")
install(DIRECTORY install/resources DESTINATION ${PKGDATADIR})

# Install locale data
if (${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.14")
# CMake 3.14 and above support TYPE LOCALE, they deduct DESTINATION themselves
install(DIRECTORY install/i18n/de TYPE LOCALE
        FILES_MATCHING PATTERN "*.mo")
else()
install(DIRECTORY install/i18n/de DESTINATION ${CMAKE_INSTALL_LOCALEDIR}
        FILES_MATCHING PATTERN "*.mo")
endif()

# Install scripts
install(DIRECTORY install/scripts DESTINATION ${PKGDATADIR}
        FILES_MATCHING PATTERN "*.py")
