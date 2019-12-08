# DarkRadiant

DarkRadiant is a level (map) editor for the **The Dark Mod**, an open-source Doom 3 modification which is available at www.thedarkmod.com. It's primary use is creating missions for The Dark Mod as well as maps for idTech4-based games like Doom 3, Quake 4 and Prey.

## Download

Get the latest DarkRadiant binaries from the [releases page](https://github.com/codereader/DarkRadiant/releases/latest). We have binaries for Windows and macOS, plus [compilation instructions](http://wiki.thedarkmod.com/index.php?title=DarkRadiant_-_Compiling_in_Linux) for various Linux distributions.

# Getting started

DarkRadiant requires game resources to work with, these resources are not installed by this editor. You'll need to point DarkRadiant to one of these games (The Dark Mod, Doom 3, Quake 4, etc.) before you can start to work on your map. Visit www.thedarkmod.com for download instructions, then proceed with one of the tutorials available on the web.

For The Dark Mod mappers, there's an extensive [Beginner's Guide](http://wiki.thedarkmod.com/index.php?title=A_-_Z_Beginner_Full_Guide_Start_Here!) on the project's wiki, which covers a lot of steps almost from scratch. Moreover, a couple of excellent video tutorials are available, linked from the top of that wiki article above.

# Compiling on Windows

## Prerequisites

DarkRadiant is built on Windows using *Microsoft Visual Studio*, the free Community Edition can be obtained here:

*VC++ 2019:* https://visualstudio.microsoft.com/downloads/ (Choose Visual Studio Community)

When installing Studio please make sure to enable the "Desktop Development with C++" workload.

Since DarkRadiant uses a couple of open-source libraries that are not available on Windows by default, you will also need to download and install the dependencies. 7-Zip packages of the dependencies are available at the following
URL(s). [Get 7-zip here](http://www.7-zip.org/)

https://github.com/codereader/DarkRadiant/releases/download/2.7.0/windeps.7z  

The dependencies packages need to be extracted into the main DarkRadiant
source directory, i.e. alongside the `include/` and `radiant/` directories.
Just drop the windeps.7z in the DarkRadiant folder and use 7-zip's "Extract to here"

## Build

The main Visual C++ solution file is located in:

`tools/msvc/DarkRadiant.sln`

Open this file with Visual Studio and start a build by right-clicking on the top-level 
"Solution 'DarkRadiant'" item and choosing Build Solution. The DarkRadiant.exe file will be 
placed into the `install/` folder.

# Compiling on Linux

## Prerequisites

To compile DarkRadiant, a number of libraries (with development headers) and a standards-compliant C++11 compiler (GCC 5.3+) are required. On an Ubuntu system, the requirements may include any or all of the following packages:

* zlib1g-dev 
* libjpeg62-dev 
* libwxgtk3.0-dev 
* libxml2-dev 
* libsigc++-2.0-dev 
* libpng-dev 
* libftgl-dev 
* libglew-dev 
* libalut-dev 
* libvorbis-dev
* pybind11-dev (Ubuntu 17 and later)

This does not include core development tools such as g++ or the git client
to download the sources (use sudo apt-get install git for that). One possible set of packages might be:

`sudo apt-get install git automake libtool g++ gettext pkg-config`

More required package lists for various Linux distributions are [listed in the Wiki Article](http://wiki.thedarkmod.com/index.php?title=DarkRadiant_-_Compiling_in_Linux).

## Build

To build DarkRadiant the standard Autotools build process is used:

```
./autogen.sh
./configure
make
sudo make install
```

The available configure options are listed with `./configure --help`. There are
options for debug builds, and enabling or disabling various optional components
such as audio support and the Dark Mod-specific plugins (`--enable-darkmod-plugins`).

# Compiling on macOS

## Prerequisites

To compile DarkRadiant, a number of libraries (with development headers) are
required. You can obtain them by using [MacPorts](https://distfiles.macports.org/MacPorts/):

Install MacPorts, then open a fresh console and issue these commands:

```
sudo port install jpeg wxwidgets-3.0 pkgconfig libsigcxx2 freetype ftgl glew
sudo port install boost libxml2 freealut libvorbis libogg openal
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

http://wiki.thedarkmod.com/index.php?title=DarkRadiant_-_Compilation_Guide

# Contact / Discussion

DarkRadiant Website: https://www.darkradiant.net

All discussion is ongoing primarily at [The Dark Mod Forums](http://forums.thedarkmod.com/forum/51-darkradiant-feedback-and-development/), where you can get in touch with knowledgeable people 
and discuss changes or issues. If you happen to run into a bug, you're encouraged to report it to us, especially when running into
application crashes (see also [How to record a crashdump](http://wiki.thedarkmod.com/index.php?title=Save_a_Memory_Dump_for_debugging_Crashes)). 

The issue tracker for DarkRadiant is also run by the Dark Mod folks: [DarkRadiant Bugtracker](http://bugs.thedarkmod.com/view_all_bug_page.php?project_id=1).

# License

The DarkRadiant source code is published under the [GNU General Public License 2.0 (GPLv2)](http://www.gnu.org/licenses/gpl-2.0.html
), except for a few libraries which are using the BSD license, see the [LICENSE](https://raw.githubusercontent.com/codereader/DarkRadiant/master/LICENSE) file for further notes.