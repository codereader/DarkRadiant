if exist ..\..\install\lib rd ..\..\install\lib /S /Q
if exist ..\..\install\etc rd ..\..\install\etc /S /Q

del ..\..\install\*.dll

copy ..\..\w64deps\openal\bin\OpenAL32.dll ..\..\install /Y
copy ..\..\w64deps\openal\bin\wrap_oal.dll ..\..\install /Y

@rem Copy wxWidgets libraries (64 bit)
IF "%1" == "Debug" GOTO WX_DEBUG
	@echo Copying wxWidgets release binaries
	xcopy ..\..\w64deps\wxWidgets\bin\wx*30u_*.dll ..\..\install /Y

GOTO WX_END

:WX_DEBUG
	@echo Copying wxWidgets debug binaries
	xcopy ..\..\w64deps\wxWidgets\bin\wx*30ud_*.dll ..\..\install /Y

:WX_END

@rem Copy FTGL library (64 bit)
IF "%1" == "Debug" GOTO FTGL_DEBUG

	copy ..\..\w64deps\ftgl\bin\ftgl-%2.dll ..\..\install /Y

GOTO FTGL_END

:FTGL_DEBUG

	copy ..\..\w64deps\ftgl\bin\ftgl-d-%2.dll ..\..\install /Y

:FTGL_END

copy ..\..\w64deps\glew\lib\glew32.dll ..\..\install /Y

copy ..\..\w64deps\python\bin\python26.dll ..\..\install /Y

@rem Copy sigc++ library (64 bit)
IF "%1" == "Debug" GOTO SIGC_DEBUG

	@echo Copying sigc++ release binary
	copy "..\..\w64deps\gtkmm\bin\libsigc++-%2.dll" ..\..\install /Y
	
GOTO SIGC_END

:SIGC_DEBUG
	@echo Copying sigc++ debug binary
	copy "..\..\w64deps\gtkmm\bin\libsigc++-d-%2.dll" ..\..\install /Y
	
:SIGC_END

@rem Copy the libxml2 x86 DLL to install
IF "%1" == "Debug" GOTO LIBXML2_DEBUG

:LIBXML2_RELEASE
	@echo Copying libxml2 release binaries
	copy ..\..\w64deps\libxml2\bin\libxml2-%2.dll ..\..\install /Y

GOTO LIBXML2_END

:LIBXML2_DEBUG
	@echo Copying libxml2 debug binaries
	copy ..\..\w64deps\libxml2\bin\libxml2-d-%2.dll ..\..\install /Y

:LIBXML2_END
