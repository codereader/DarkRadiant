msbuild ..\msvc2012\DarkRadiant.sln /p:configuration=release /t:rebuild /p:platform=x64 /maxcpucount:4 
call copy_install_files.x64.cmd
call ..\innosetup\create_installer.x64.cmd