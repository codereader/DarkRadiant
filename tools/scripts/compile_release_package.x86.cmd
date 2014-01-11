msbuild ..\msvc2012\DarkRadiant.sln /p:configuration=release /t:rebuild /p:platform=Win32 /maxcpucount:4 
call copy_install_files.cmd
call ..\innosetup\create_installer.x86.cmd