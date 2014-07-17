md ..\..\..\DarkRadiant_install
del ..\..\..\DarkRadiant_install\*.* /S /Q
xcopy ..\..\install\*.* /s ..\..\..\DarkRadiant_install\
del ..\..\..\DarkRadiant_install\*.exp /S /Q
rem del ..\..\..\DarkRadiant_install\*.pdb /S /Q
del ..\..\..\DarkRadiant_install\*.lib /S /Q
del ..\..\..\DarkRadiant_install\*.suo /S /Q
del ..\..\..\DarkRadiant_install\*.pgd /S /Q
del ..\..\..\DarkRadiant_install\*.fbp /S /Q
del ..\..\..\DarkRadiant_install\darkradiant.desktop.in