msbuild ..\msvc2013\DarkRadiant.sln /p:configuration=release /t:rebuild /p:platform=Win32 /maxcpucount:4
call copy_install_files.cmd
cd ..\innosetup
@echo For this script to run, please make sure that InnoSetup's compil32 is found via the PATH environment variable
compil32 /cc darkradiant.iss