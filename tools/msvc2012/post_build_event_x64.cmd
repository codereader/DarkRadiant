if exist ..\..\install\lib rd ..\..\install\lib /S /Q
if exist ..\..\install\etc rd ..\..\install\etc /S /Q

del ..\..\install\*.dll

copy ..\..\w64deps\openal\bin\OpenAL32.dll ..\..\install /Y
copy ..\..\w64deps\openal\bin\wrap_oal.dll ..\..\install /Y

copy ..\..\w32deps\wxWidgets\lib\vc110_x64_dll\*.dll ..\..\install /Y

@rem Copy GTK+ libraries (64 bit)
IF "%1" == "Debug" GOTO GTK_DEBUG

	copy ..\..\w64deps\ftgl\bin\ftgl-%2.dll ..\..\install /Y

GOTO GTK_END

:GTK_DEBUG

	copy ..\..\w64deps\ftgl\bin\ftgl-d-%2.dll ..\..\install /Y

:GTK_END

copy ..\..\w64deps\glew\lib\glew32.dll ..\..\install /Y

copy ..\..\w64deps\python\bin\python26.dll ..\..\install /Y

@rem Copy gtkmm libraries (64 bit)
IF "%1" == "Debug" GOTO GTKMM_DEBUG

:GTKMM_RELEASE
	@echo Copying GTKmm release binaries
	copy "..\..\w64deps\gtkmm\bin\libsigc++-%2.dll" ..\..\install /Y
	
GOTO GTKMM_END

:GTKMM_DEBUG
	@echo Copying GTKmm debug binaries
	copy "..\..\w64deps\gtkmm\bin\libsigc++-d-%2.dll" ..\..\install /Y
	
:GTKMM_END

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
