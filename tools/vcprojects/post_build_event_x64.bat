copy ..\..\w32deps\libxml2\lib\libxml2.dll ..\..\install

copy ..\..\w64deps\openal\bin\OpenAL32.dll ..\..\install
copy ..\..\w64deps\openal\bin\wrap_oal.dll ..\..\install

rem Copy everything from w32deps etc folder
md ..\..\install\etc
xcopy ..\..\w32deps\gtk2\bin\etc\* ..\..\install\etc\. /S /Y

rem The LIB folder containing the engines should come from the w64deps folder
md ..\..\install\lib
xcopy ..\..\w64deps\gtk2\bin\lib\* ..\..\install\lib\. /S /Y

copy ..\..\w64deps\gtk2\bin\libatk-1.0-0.dll ..\..\install
copy ..\..\w64deps\gtk2\bin\libcairo-2.dll ..\..\install
copy ..\..\w64deps\gtk2\bin\libjpeg-62.dll ..\..\install
copy ..\..\w64deps\gtk2\bin\libpng12-0.dll ..\..\install
copy ..\..\w64deps\gtk2\bin\libtiff.dll ..\..\install
copy ..\..\w64deps\gtk2\bin\libgio-2.0-0.dll ..\..\install
copy ..\..\w64deps\gtk2\bin\libgdk_pixbuf-2.0-0.dll ..\..\install
copy ..\..\w64deps\gtk2\bin\libgdk-win32-2.0-0.dll ..\..\install
copy ..\..\w64deps\gtk2\bin\libglib-2.0-0.dll ..\..\install
copy ..\..\w64deps\gtk2\bin\libgmodule-2.0-0.dll ..\..\install
copy ..\..\w64deps\gtk2\bin\libgobject-2.0-0.dll ..\..\install
copy ..\..\w64deps\gtk2\bin\libgtk-win32-2.0-0.dll ..\..\install
copy ..\..\w64deps\gtk2\bin\libpango-1.0-0.dll ..\..\install
copy ..\..\w64deps\gtk2\bin\zlib1.dll ..\..\install
copy ..\..\w64deps\gtk2\bin\libpangocairo-1.0-0.dll ..\..\install
copy ..\..\w64deps\gtk2\bin\libpangowin32-1.0-0.dll ..\..\install

copy ..\..\w64deps\glew\lib\glew32.dll ..\..\install