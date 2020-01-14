del ..\..\install\*.dll
del ..\..\install\python*.zip

copy ..\..\w64deps\openal\bin\OpenAL32.dll ..\..\install /Y
copy ..\..\w64deps\openal\bin\wrap_oal.dll ..\..\install /Y
copy ..\..\w64deps\glew\lib\glew32.dll ..\..\install /Y
copy ..\..\w64deps\python\bin\python3*.dll ..\..\install /Y
copy ..\..\w64deps\python\bin\python3*.zip ..\..\install /Y

@rem Copy wxWidgets libraries (64 bit)

IF "%1" == "Debug" (SET WXLIB_SUFFIX=ud) ELSE (SET WXLIB_SUFFIX=u)

@echo Copying wxWidgets binaries

copy ..\..\w64deps\wxWidgets\bin\wxmsw313%WXLIB_SUFFIX%_gl_vc*_x64.dll	   ..\..\install /Y
copy ..\..\w64deps\wxWidgets\bin\wxmsw313%WXLIB_SUFFIX%_html_vc*_x64.dll  ..\..\install /Y
copy ..\..\w64deps\wxWidgets\bin\wxmsw313%WXLIB_SUFFIX%_xrc_vc*_x64.dll   ..\..\install /Y
copy ..\..\w64deps\wxWidgets\bin\wxmsw313%WXLIB_SUFFIX%_stc_vc*_x64.dll   ..\..\install /Y
copy ..\..\w64deps\wxWidgets\bin\wxbase313%WXLIB_SUFFIX%_vc*_x64.dll	   ..\..\install /Y
copy ..\..\w64deps\wxWidgets\bin\wxbase313%WXLIB_SUFFIX%_xml_vc*_x64.dll  ..\..\install /Y
copy ..\..\w64deps\wxWidgets\bin\wxmsw313%WXLIB_SUFFIX%_adv_vc*_x64.dll   ..\..\install /Y
copy ..\..\w64deps\wxWidgets\bin\wxmsw313%WXLIB_SUFFIX%_core_vc*_x64.dll  ..\..\install /Y

IF "%1" == "Debug" (SET DEBUG_SUFFIX=-d) ELSE (SET DEBUG_SUFFIX=)

@echo Copying FTGL library (64 bit)
copy ..\..\w64deps\ftgl\bin\ftgl%DEBUG_SUFFIX%-%2.dll ..\..\install /Y

@echo Copying sigc++ library (64 bit)
copy "..\..\w64deps\libsigc++\bin\libsigc++%DEBUG_SUFFIX%-%2.dll" ..\..\install /Y

@echo Copying libxml2 library (64 bit)
copy ..\..\w64deps\libxml2\bin\libxml2%DEBUG_SUFFIX%-%2.dll ..\..\install /Y