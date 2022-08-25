for %%i in (..\..\install\*.dll) do del "%%i"

del ..\..\install\python*.zip

copy ..\..\windeps\openal\bin\OpenAL32.dll ..\..\install /Y
copy ..\..\windeps\openal\bin\wrap_oal.dll ..\..\install /Y
copy ..\..\windeps\glew\bin\glew32.dll ..\..\install /Y
copy ..\..\windeps\python\bin\python3*.dll ..\..\install /Y
copy ..\..\windeps\python\bin\python3*.zip ..\..\install /Y

@rem Copy wxWidgets libraries (64 bit)

IF "%1" == "Debug" (SET WXLIB_SUFFIX=ud) ELSE (SET WXLIB_SUFFIX=u)

@echo Copying wxWidgets binaries

copy ..\..\windeps\wxWidgets\bin\wxmsw32%WXLIB_SUFFIX%_gl_vc*_x64.dll	   ..\..\install /Y
copy ..\..\windeps\wxWidgets\bin\wxmsw32%WXLIB_SUFFIX%_html_vc*_x64.dll  ..\..\install /Y
copy ..\..\windeps\wxWidgets\bin\wxmsw32%WXLIB_SUFFIX%_xrc_vc*_x64.dll   ..\..\install /Y
copy ..\..\windeps\wxWidgets\bin\wxmsw32%WXLIB_SUFFIX%_stc_vc*_x64.dll   ..\..\install /Y
copy ..\..\windeps\wxWidgets\bin\wxbase32%WXLIB_SUFFIX%_vc*_x64.dll	   ..\..\install /Y
copy ..\..\windeps\wxWidgets\bin\wxbase32%WXLIB_SUFFIX%_xml_vc*_x64.dll  ..\..\install /Y
copy ..\..\windeps\wxWidgets\bin\wxmsw32%WXLIB_SUFFIX%_adv_vc*_x64.dll   ..\..\install /Y
copy ..\..\windeps\wxWidgets\bin\wxmsw32%WXLIB_SUFFIX%_core_vc*_x64.dll  ..\..\install /Y
copy ..\..\windeps\wxWidgets\bin\wxmsw32%WXLIB_SUFFIX%_aui_vc*_x64.dll  ..\..\install /Y
copy ..\..\windeps\wxWidgets\bin\wxbase32%WXLIB_SUFFIX%_net_vc*_x64.dll  ..\..\install /Y

IF "%1" == "Debug" (SET DEBUG_SUFFIX=-d) ELSE (SET DEBUG_SUFFIX=)

@echo Copying FTGL library (64 bit)
copy ..\..\windeps\ftgl\bin\ftgl%DEBUG_SUFFIX%-%2.dll ..\..\install /Y

@echo Copying sigc++ library (64 bit)
copy "..\..\windeps\libsigc++\bin\libsigc++%DEBUG_SUFFIX%-%2.dll" ..\..\install /Y

@echo Copying libxml2 library (64 bit)
copy ..\..\windeps\libxml2\bin\libxml2%DEBUG_SUFFIX%-%2.dll ..\..\install /Y

@echo Copying libgit2 library (64-bit)
copy ..\..\windeps\libgit2\bin\libgit2%DEBUG_SUFFIX%.dll ..\..\install /Y