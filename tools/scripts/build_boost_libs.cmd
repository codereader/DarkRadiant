@echo off
rem Launch this file in the boost/ folder.

rem Check if we're in the correct folder
if not exist libs goto :error
if not exist boost goto :error
if not exist b2.exe goto :error

mkdir stage

b2 toolset=msvc variant=release,debug link=static threading=multi stage /boost/python /boost/filesystem /boost/regex /boost/system

start stage

goto :success

:error
echo Please launch this file in the boost folder you downloaded and extracted from sourceforge.
echo Run the bootstrap.bat file to generate the b2.exe file needed for the build process.
echo __________________________________________________________________________________________
echo Example: 
echo   cd c:\Downloads\boost_1_61_0\
echo   c:\Games\DarkRadiant\tools\scripts\build_boost_libs.cmd
goto :eof

:success
echo Successfully built the libraries, please copy them to the w32deps/boost/libs/ folder now.