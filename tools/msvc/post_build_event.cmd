del ..\..\install\*.dll
del ..\..\install\python*.zip

copy ..\..\w32deps\openal\bin\OpenAL32.dll ..\..\install /Y
copy ..\..\w32deps\openal\bin\wrap_oal.dll ..\..\install /Y
copy ..\..\w32deps\glew\bin\glew32.dll ..\..\install /Y
copy ..\..\w32deps\python\bin\python3*.dll ..\..\install /Y
copy ..\..\w32deps\python\bin\python3*.zip ..\..\install /Y

@rem Copy wxWidgets libraries (32 bit)

IF "%1" == "Debug" (SET WXLIB_SUFFIX=ud) ELSE (SET WXLIB_SUFFIX=u)

@echo Copying wxWidgets binaries

copy ..\..\w32deps\wxWidgets\bin\wxmsw313%WXLIB_SUFFIX%_gl_vc*.dll	   ..\..\install /Y
copy ..\..\w32deps\wxWidgets\bin\wxmsw313%WXLIB_SUFFIX%_html_vc*.dll  ..\..\install /Y
copy ..\..\w32deps\wxWidgets\bin\wxmsw313%WXLIB_SUFFIX%_xrc_vc*.dll   ..\..\install /Y
copy ..\..\w32deps\wxWidgets\bin\wxmsw313%WXLIB_SUFFIX%_stc_vc*.dll   ..\..\install /Y
copy ..\..\w32deps\wxWidgets\bin\wxbase313%WXLIB_SUFFIX%_vc*.dll	   ..\..\install /Y
copy ..\..\w32deps\wxWidgets\bin\wxbase313%WXLIB_SUFFIX%_xml_vc*.dll  ..\..\install /Y
copy ..\..\w32deps\wxWidgets\bin\wxmsw313%WXLIB_SUFFIX%_adv_vc*.dll   ..\..\install /Y
copy ..\..\w32deps\wxWidgets\bin\wxmsw313%WXLIB_SUFFIX%_core_vc*.dll  ..\..\install /Y

IF "%1" == "Debug" (SET DEBUG_SUFFIX=-d) ELSE (SET DEBUG_SUFFIX=)

@echo Copying FTGL library (32 bit)
copy ..\..\w32deps\ftgl\bin\ftgl%DEBUG_SUFFIX%-%2.dll ..\..\install /Y

@echo Copying sigc++ library (32 bit)
copy "..\..\w32deps\libsigc++\bin\libsigc++%DEBUG_SUFFIX%-%2.dll" ..\..\install /Y

@echo Copying libxml2 library (32 bit)
copy ..\..\w32deps\libxml2\bin\libxml2%DEBUG_SUFFIX%-%2.dll ..\..\install /Y
