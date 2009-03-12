cd ..
cd ..
cd ..
md DarkRadiant_install.x64
cd DarkRadiant_install.x64
del *.* /S /Q
xcopy ..\DarkRadiant\install\*.* /s .
del *.exp /S /Q
del *.pdb /S /Q
del *.lib /S /Q
del *.suo /S /Q
del *.pgd /S /Q

cd ..
cd DarkRadiant\tools\innosetup
start darkradiant.x64.iss