cd ..
cd ..
cd ..
md DarkRadiant_install
cd DarkRadiant_install
del *.* /S /Q
xcopy ..\DarkRadiant\install\*.* /s .
del *.exp /S /Q
rem del *.pdb /S /Q
del *.lib /S /Q
del *.suo /S /Q
del *.pgd /S /Q
del darkradiant.desktop.in

cd ..
cd DarkRadiant\tools\innosetup
start darkradiant.iss