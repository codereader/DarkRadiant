cmake_minimum_required(VERSION 3.12)

# Project name and version
project(darkradiant VERSION 2.9.2)

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Build shared libraries by default
option(BUILD_SHARED_LIBS "Build shared libraries" ON)
option(SKIP_DM_PLUGINS "Don't build Dark Mod specific plugins" OFF)
set(CMAKE_INSTALL_RPATH "$ORIGIN/../lib/darkradiant")

# Debug or release mode
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
add_compile_definitions(
    $<$<CONFIG:Debug>:_DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
)

# Define GNU-style directory structure by default, and pass directories to the
# code with defines
include(GNUInstallDirs)

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

# Locate wxWidgets
find_package(wxWidgets REQUIRED
             COMPONENTS base core stc adv gl xrc)
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
set(CORE_MODULE_LIBRARY "libradiantcore")
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

# Dark Mod plugins
if (NOT SKIP_DM_PLUGINS)
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

# Install main targets
install(TARGETS darkradiant math xmlutil scenegraph wxutil
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib/darkradiant)
install(TARGETS radiantcore script sound
        LIBRARY DESTINATION lib/darkradiant/modules)

# Install Dark Mod plugins
if (NOT SKIP_DM_PLUGINS)
    install(TARGETS dm_stimresponse dm_objectives dm_difficulty dm_editing
                    dm_gui dm_gameconnection
            LIBRARY DESTINATION lib/darkradiant/plugins)
endif()

# Generate and install the .desktop file
configure_file(install/darkradiant.desktop.in install/darkradiant.desktop)
install(FILES install/darkradiant.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)

# Install resources
set(PKG_DATA_DIR "${CMAKE_INSTALL_DATADIR}/darkradiant")

file(GLOB XML_FILES "${PROJECT_SOURCE_DIR}/install/*.xml")
install(FILES ${XML_FILES} DESTINATION ${PKG_DATA_DIR})

install(DIRECTORY install/games DESTINATION ${PKG_DATA_DIR})
install(DIRECTORY install/bitmaps DESTINATION ${PKG_DATA_DIR})
install(DIRECTORY install/gl DESTINATION ${PKG_DATA_DIR})
install(DIRECTORY install/ui DESTINATION ${PKG_DATA_DIR}
        FILES_MATCHING PATTERN "*.ttf" PATTERN "*.xrc")

# Install scripts
install(DIRECTORY install/scripts DESTINATION ${PKG_DATA_DIR}
        FILES_MATCHING PATTERN "*.py")
