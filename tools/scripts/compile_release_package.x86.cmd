msbuild ..\msvc2013\DarkRadiant.sln /p:configuration=release /t:rebuild /p:platform=Win32 /maxcpucount:4
call copy_install_files.cmd
cd ..\innosetup
@echo For this script to run, please make sure that InnoSetup's compil32 is found via the PATH environment variable
compil32 /cc darkradiant.iss
"C:\Program Files\7-Zip\7z.exe" a -r -x!*.pdb -mx9 ..\innosetup\darkradiant-X.Y.ZpreV-x86.7z ..\..\..\DarkRadiant_install\*.*
"C:\Program Files\7-Zip\7z.exe" a -r -mx9 ..\innosetup\darkradiant-X.Y.Zpre5-x86.pdb.7z ..\..\..\DarkRadiant_install\*.pdb