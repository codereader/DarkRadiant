# DarkRadiant

DarkRadiant is a level (map) editor for the **The Dark Mod**, an open-source Doom 3
modification which is available at http://www.thedarkmod.com

# Compiling on Windows

## Prerequisites

DarkRadiant is built on Windows using *Microsoft Visual C++ 2015*. 
The free Community Edition can be obtained here:

*VC++ 2015:* https://www.visualstudio.com/en-us/downloads (Choose Visual Studio Community)

Since DarkRadiant uses a couple of open-source libraries that are not available on
Windows by default, you will also need to download and install the
dependencies. 7-Zip packages of the dependencies are available at the following
URL(s). [Get 7-zip here](http://www.7-zip.org/)

*32-bit only builds:*  
https://github.com/codereader/DarkRadiant/releases/download/2.1.0/w32deps.7z

*64-bit builds:*  
https://github.com/codereader/DarkRadiant/releases/download/2.1.0/w32deps.7z  
https://github.com/codereader/DarkRadiant/releases/download/2.1.0/w64deps.7z  

Note that 64-bit builds need the 32-bit dependencies in addition to their own
64-bit dependencies.

The dependencies packages need to be extracted into the main DarkRadiant
source directory, i.e. alongside the **include/** and **radiant/** directories.

## Build

The main Visual C++ solution file is:

Visual Studio 2015: **tools/msvc2015/DarkRadiant.sln**

Open this file with Visual Studio and start a build by right-clicking on the top-level 
"Solution 'DarkRadiant'" item and choosing Build Solution. The DarkRadiant.exe file will be 
placed into the **install/** folder.

# Compiling on Linux

## Prerequisites

To compile DarkRadiant, a number of libraries (with development headers) are
required. On an Ubuntu system, the requirements may include any or all of the
following packages:

* zlib1g-dev 
* libjpeg62-dev 
* libwxgtk3.0-dev
* libxml2-dev
* libsigc++-2.0-dev
* libpng12-dev
* ftgl-dev
* libglew-dev
* libboost-dev
* libboost-regex-dev
* libboost-filesystem-dev 
* libboost-python-dev
* libboost-test-dev
* libalut-dev 
* libvorbis-dev 
* python-dev

This does not include core development tools such as g++ or the git client
to download the sources (use sudo apt-get install git for that). One possible
set of packages might be:

`sudo apt-get install git automake libtool g++`

## Build

To build DarkRadiant the standard Autotools build process is used:

```./autogen.sh
./configure
make
sudo make install
```

The available configure options are listed with `./configure --help`. There are
options for debug builds, and enabling or disabling various optional components
such as audio support and the Dark Mod-specific plugins.

# More Information

A more detailed compilation guide can be found on The Dark Mod's wiki:

http://wiki.thedarkmod.com/index.php?title=DarkRadiant_-_Compilation_Guide

# Contact / Discussion

All discussion is ongoing primarily at [The Dark Mod Forums](http://forums.thedarkmod.com/forum/51-darkradiant-feedback-and-development/), where you can get in touch with knowledgeable people 
and discuss changes or issues. If you happen to run into a bug, you're encouraged to report it to us, especially when running into
application crashes (see also [How to record a crashdump](http://wiki.thedarkmod.com/index.php?title=Save_a_Memory_Dump_for_debugging_Crashes)). 

The issue tracker for DarkRadiant is also run by the Dark Mod folks: [DarkRadiant Bugtracker](http://bugs.thedarkmod.com/view_all_bug_page.php?project_id=1).
