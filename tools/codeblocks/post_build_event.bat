del ..\..\install\lib\* /S /Q
rd ..\..\install\lib
del ..\..\install\etc\* /S /Q
rd ..\..\install\etc
del ..\..\install\*.dll

copy ..\..\w32deps\libxml2\lib\libxml2.dll ..\..\install

copy ..\..\w32deps\openal\bin\OpenAL32.dll ..\..\install
copy ..\..\w32deps\openal\bin\wrap_oal.dll ..\..\install

copy ..\..\w32deps\glew\bin\glew32.dll ..\..\install

copy ..\..\w32deps\libpng\bin\libpng13.dll ..\..\install
copy ..\..\w32deps\libiconv\bin\libiconv2.dll ..\..\install

md ..\..\install\etc
xcopy ..\..\w32deps\gtk2\bin\etc\* ..\..\install\etc\. /S /Y

md ..\..\install\lib
xcopy ..\..\w32deps\gtk2\bin\lib\* ..\..\install\lib\. /S /Y

copy ..\..\w32deps\gtk2\bin\iconv.dll ..\..\install
copy ..\..\w32deps\gtk2\bin\intl.dll ..\..\install
copy ..\..\w32deps\gtk2\bin\jpeg62.dll ..\..\install
copy ..\..\w32deps\gtk2\bin\libatk-1.0-0.dll ..\..\install
copy ..\..\w32deps\gtk2\bin\libgailutil-18.dll ..\..\install
copy ..\..\w32deps\gtk2\bin\libcairo-2.dll ..\..\install
rem copy ..\..\w32deps\gtk2\bin\libfontconfig-1.dll ..\..\install
copy ..\..\w32deps\gtk2\bin\libgdk_pixbuf-2.0-0.dll ..\..\install
copy ..\..\w32deps\gtk2\bin\libgdk-win32-2.0-0.dll ..\..\install
copy ..\..\w32deps\gtk2\bin\libgio-2.0-0.dll ..\..\install
copy ..\..\w32deps\gtk2\bin\libglib-2.0-0.dll ..\..\install
copy ..\..\w32deps\gtk2\bin\libgmodule-2.0-0.dll ..\..\install
copy ..\..\w32deps\gtk2\bin\libgobject-2.0-0.dll ..\..\install
copy ..\..\w32deps\gtk2\bin\libgthread-2.0-0.dll ..\..\install
copy ..\..\w32deps\gtk2\bin\libgtk-win32-2.0-0.dll ..\..\install
copy ..\..\w32deps\gtk2\bin\libpango-1.0-0.dll ..\..\install
copy ..\..\w32deps\gtk2\bin\libpangocairo-1.0-0.dll ..\..\install
copy ..\..\w32deps\gtk2\bin\libpangoft2-1.0-0.dll ..\..\install
copy ..\..\w32deps\gtk2\bin\libpangowin32-1.0-0.dll ..\..\install
copy ..\..\w32deps\gtk2\bin\libpng12-0.dll ..\..\install
copy ..\..\w32deps\gtk2\bin\libtiff3.dll ..\..\install
copy ..\..\w32deps\gtk2\bin\zlib1.dll ..\..\install

copy ..\..\w32deps\python\bin\python26.dll ..\..\install