<img align="right" src="https://github.com/codereader/DarkRadiant/blob/master/install/bitmaps/repository_logo.png?raw=true" alt="DarkRadiant Logo">

# DarkRadiant

DarkRadiant is a level (map) editor for the **The Dark Mod**, an open-source Doom 3 modification which is available at www.thedarkmod.com. Its primary use is creating missions for The Dark Mod as well as maps for idTech4-based games like Doom 3, Quake 4 and Prey.

## Download

Get the latest DarkRadiant binaries from the [releases page](https://github.com/codereader/DarkRadiant/releases/latest). We have binaries for Windows and macOS, plus [compilation instructions](https://wiki.thedarkmod.com/index.php?title=DarkRadiant_-_Compiling_in_Linux) for various Linux distributions.

# Getting started

DarkRadiant requires game resources to work with, these resources are not installed by this editor. You'll need to point DarkRadiant to one of these games (The Dark Mod, Doom 3, Quake 4, etc.) before you can start to work on your map. Visit [www.thedarkmod.com](https://www.thedarkmod.com) for download instructions, then proceed with one of the tutorials available on the web.

For The Dark Mod mappers, there are a couple of [Video Tutorials](https://wiki.thedarkmod.com/index.php?title=DarkRadiant_Video_Tutorials) on the project's wiki, which should get you started.

# Compiling on Windows

## Prerequisites

DarkRadiant is built on Windows using *Microsoft Visual Studio*, the free Community Edition can be obtained here:

*VC++ 2022:* https://visualstudio.microsoft.com/downloads/ (Choose Visual Studio Community)

When installing Studio please make sure to enable the "Desktop Development with C++" workload.

## Build

The main Visual C++ solution file is located in the root folder of this repository:

`DarkRadiant.sln`

Open this file with Visual Studio and start a build by right-clicking on the top-level 
"Solution 'DarkRadiant'" item and choosing Build Solution. The `DarkRadiant.exe` file will be placed in the `install/` folder.

### Windows Build Dependencies

Since DarkRadiant requires a couple of open-source libraries that are not available on Windows by default, it will try to download and install the dependencies when the build starts. If it fails for some reason, you can try to run this script:

 `tools/scripts/download_windeps.ps1`

or extract the tools manually, downloading the 7-Zip package containing the necessary files from the URL below ([Get 7-zip here](https://www.7-zip.org/)):

https://github.com/codereader/DarkRadiant_WinDeps/releases/latest/  

The dependencies packages need to be extracted into the main DarkRadiant source directory, i.e. alongside the `include/` and `radiant/` directories.
Just drop the windeps.7z in the DarkRadiant folder and use 7-zip's "Extract to here"

# Compiling on Linux

## Prerequisites

To compile DarkRadiant a number of libraries (with development headers) and a standards-compliant C++17 compiler are required. On an Ubuntu system the requirements may include any or all of the following packages:

* zlib1g-dev 
* libjpeg-dev 
* libwxgtk3.0-dev 
* libxml2-dev 
* libsigc++-2.0-dev 
* libpng-dev 
* libftgl-dev 
* libglew-dev 
* libalut-dev 
* libvorbis-dev
* libgtest-dev
* libeigen3-dev
* libgit2-dev (optional)

This does not include core development tools such as g++ or the git client
to download the sources (use sudo apt-get install git for that). One possible set of packages might be:

`sudo apt-get install git cmake g++ gettext pkg-config`

More required package lists for various Linux distributions are [listed in the Wiki Article](https://wiki.thedarkmod.com/index.php?title=DarkRadiant_-_Compiling_in_Linux).

## Build

To build DarkRadiant the standard CMake build process is used:

```
cmake .
make
sudo make install
```

To install somewhere other than the default of `/usr/local`, use the `CMAKE_INSTALL_PREFIX` variable.

```
cmake -DCMAKE_INSTALL_PREFIX=/opt/darkradiant
make
sudo make install
```

Other useful variables are `CMAKE_BUILD_TYPE` to choose Debug or Release builds, `ENABLE_DM_PLUGINS` to disable the building of Dark Mod specific plugins (enabled by default), and `ENABLE_RELOCATION` to control whether DarkRadiant uses hard-coded absolute paths like `/usr/lib` or paths relative to the binary (useful for certain package formats like Snappy or FlatPak).

# Compiling on macOS

## Prerequisites

You'll need an Xcode version supporting C++17 and the macOS 10.15 (Catalina) target at minimum. Xcode 11.3 should be working fine. You will need to install the Xcode command line tools to install MacPorts (run `xcode-select --install`)

To compile DarkRadiant, a number of libraries (with development headers) are
required. You can obtain them by using [MacPorts](https://distfiles.macports.org/MacPorts/):

Install MacPorts, then open a fresh console and issue these commands:

```
sudo port install jpeg wxwidgets-3.0 pkgconfig libsigcxx2 freetype ftgl glew
sudo port install libxml2 freealut libvorbis libogg openal-soft eigen3
```

## Build

Start Xcode and open the project file in `tools/xcode/DarkRadiant.xcodeproj`.
Hit CMD-B to start the build, the output files will be placed to a folder
similar to this:

`~/Library/Developer/Xcode/DerivedData/DarkRadiant-somethingsomething/Build/Products/Release`

The `DarkRadiant.app` package in that folder can be launched right away or
copied to some location of your preference.

# More Build Information

A more detailed compilation guide can be found on The Dark Mod's wiki:

https://wiki.thedarkmod.com/index.php?title=DarkRadiant_-_Compilation_Guide

# Contact / Discussion

DarkRadiant Website: https://www.darkradiant.net

All discussion is ongoing primarily at [The Dark Mod Forums](https://forums.thedarkmod.com/forum/51-darkradiant-feedback-and-development/), where you can get in touch with knowledgeable people 
and discuss changes or issues. If you happen to run into a bug, you're encouraged to report it to us, especially when running into
application crashes (see also [How to record a crashdump](https://wiki.thedarkmod.com/index.php?title=Save_a_Memory_Dump_for_debugging_Crashes)). 

The issue tracker for DarkRadiant is also run by the Dark Mod folks: [DarkRadiant Bugtracker](https://bugs.thedarkmod.com/view_all_bug_page.php?project_id=1).

# License

The DarkRadiant source code is published under the [GNU General Public License 2.0 (GPLv2)](http://www.gnu.org/licenses/gpl-2.0.html
), except for a few libraries which are using the BSD or MIT licenses, see the [LICENSE](https://raw.githubusercontent.com/codereader/DarkRadiant/master/LICENSE) file for specifics.
