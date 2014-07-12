del ..\..\install\*.dll

copy ..\..\w32deps\openal\bin\OpenAL32.dll ..\..\install /Y
copy ..\..\w32deps\openal\bin\wrap_oal.dll ..\..\install /Y
copy ..\..\w32deps\glew\bin\glew32.dll ..\..\install /Y
copy ..\..\w32deps\python\bin\python26.dll ..\..\install /Y

@rem Copy wxWidgets libraries (32 bit)

IF "%1" == "Debug" (SET WXLIB_SUFFIX=ud) ELSE (SET WXLIB_SUFFIX=u)

@echo Copying wxWidgets binaries

copy ..\..\w32deps\wxWidgets\bin\wxmsw30%WXLIB_SUFFIX%_gl_%2.dll	   ..\..\install /Y
copy ..\..\w32deps\wxWidgets\bin\wxmsw30%WXLIB_SUFFIX%_html_%2.dll  ..\..\install /Y
copy ..\..\w32deps\wxWidgets\bin\wxmsw30%WXLIB_SUFFIX%_xrc_%2.dll   ..\..\install /Y
copy ..\..\w32deps\wxWidgets\bin\wxmsw30%WXLIB_SUFFIX%_stc_%2.dll   ..\..\install /Y
copy ..\..\w32deps\wxWidgets\bin\wxbase30%WXLIB_SUFFIX%_%2.dll	   ..\..\install /Y
copy ..\..\w32deps\wxWidgets\bin\wxbase30%WXLIB_SUFFIX%_xml_%2.dll  ..\..\install /Y
copy ..\..\w32deps\wxWidgets\bin\wxmsw30%WXLIB_SUFFIX%_adv_%2.dll   ..\..\install /Y
copy ..\..\w32deps\wxWidgets\bin\wxmsw30%WXLIB_SUFFIX%_core_%2.dll  ..\..\install /Y

IF "%1" == "Debug" (SET DEBUG_SUFFIX=-d) ELSE (SET DEBUG_SUFFIX=)

@echo Copying FTGL library (32 bit)
copy ..\..\w32deps\ftgl\bin\ftgl%DEBUG_SUFFIX%-%2.dll ..\..\install /Y

@echo Copying sigc++ library (32 bit)
copy "..\..\w32deps\libsigc++\bin\libsigc++%DEBUG_SUFFIX%-%2.dll" ..\..\install /Y

@echo Copying libxml2 library (32 bit)
copy ..\..\w32deps\libxml2\bin\libxml2%DEBUG_SUFFIX%-%2.dll ..\..\install /Y
