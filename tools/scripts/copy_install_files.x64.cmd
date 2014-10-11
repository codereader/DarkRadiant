md ..\..\..\DarkRadiant_install.x64
del ..\..\..\DarkRadiant_install.x64\*.* /S /Q
xcopy ..\..\install\*.* /s ..\..\..\DarkRadiant_install.x64\
del ..\..\..\DarkRadiant_install.x64\*.exp /S /Q
rem del ..\..\..\DarkRadiant_install.x64\*.pdb /S /Q
del ..\..\..\DarkRadiant_install.x64\*.lib /S /Q
del ..\..\..\DarkRadiant_install.x64\*.suo /S /Q
del ..\..\..\DarkRadiant_install.x64\*.pgd /S /Q
del ..\..\..\DarkRadiant_install.x64\*.fbp /S /Q
del ..\..\..\DarkRadiant_install.x64\darkradiant.desktop.in