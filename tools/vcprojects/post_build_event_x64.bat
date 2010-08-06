if exist ..\..\install\lib rd ..\..\install\lib /S /Q
if exist ..\..\install\etc rd ..\..\install\etc /S /Q

del ..\..\install\*.dll

copy ..\..\w64deps\openal\bin\OpenAL32.dll ..\..\install /Y
copy ..\..\w64deps\openal\bin\wrap_oal.dll ..\..\install /Y

@rem Copy everything from w32deps etc folder
md ..\..\install\etc
xcopy ..\..\w32deps\gtk2\bin\etc\* ..\..\install\etc\. /S /Y

@rem The LIB folder containing the engines should come from the w64deps folder
md ..\..\install\lib
xcopy ..\..\w64deps\gtk2\bin\lib\* ..\..\install\lib\. /S /Y

copy ..\..\w64deps\gtk2\bin\libatk-1.0-0.dll ..\..\install /Y
copy ..\..\w64deps\gtk2\bin\libcairo-2.dll ..\..\install /Y
copy ..\..\w64deps\gtk2\bin\libjpeg-62.dll ..\..\install /Y
copy ..\..\w64deps\gtk2\bin\libpng12-0.dll ..\..\install /Y
copy ..\..\w64deps\gtk2\bin\libtiff.dll ..\..\install /Y 
copy ..\..\w64deps\gtk2\bin\libgio-2.0-0.dll ..\..\install /Y
copy ..\..\w64deps\gtk2\bin\libgdk_pixbuf-2.0-0.dll ..\..\install /Y
copy ..\..\w64deps\gtk2\bin\libgdk-win32-2.0-0.dll ..\..\install /Y
copy ..\..\w64deps\gtk2\bin\libglib-2.0-0.dll ..\..\install /Y
copy ..\..\w64deps\gtk2\bin\libgmodule-2.0-0.dll ..\..\install /Y
copy ..\..\w64deps\gtk2\bin\libgobject-2.0-0.dll ..\..\install /Y
copy ..\..\w64deps\gtk2\bin\libgtk-win32-2.0-0.dll ..\..\install /Y
copy ..\..\w64deps\gtk2\bin\libpango-1.0-0.dll ..\..\install /Y
copy ..\..\w64deps\gtk2\bin\zlib1.dll ..\..\install /Y
copy ..\..\w64deps\gtk2\bin\libpangocairo-1.0-0.dll ..\..\install /Y
copy ..\..\w64deps\gtk2\bin\libpangowin32-1.0-0.dll ..\..\install /Y
copy ..\..\w64deps\gtk2\bin\libpangoft2-1.0-0.dll ..\..\install /Y
copy ..\..\w64deps\gtk2\bin\libintl-8.dll ..\..\install /Y

copy ..\..\w64deps\glew\lib\glew32.dll ..\..\install /Y

copy ..\..\w64deps\python\bin\python26.dll ..\..\install /Y

@rem Copy gtkmm libraries (64 bit)
IF "%1" == "Debug" GOTO GTKMM_DEBUG

:GTKMM_RELEASE
	@echo Copying GTKmm release binaries
	copy ..\..\w64deps\gtkmm\bin\atkmm-vc90-1_6.dll ..\..\install /Y
	copy ..\..\w64deps\gtkmm\bin\cairomm-vc90-1_0.dll ..\..\install /Y
	copy ..\..\w64deps\gtkmm\bin\gdkmm-vc90-2_4.dll ..\..\install /Y
	copy ..\..\w64deps\gtkmm\bin\giomm-vc90-2_4.dll ..\..\install /Y
	copy ..\..\w64deps\gtkmm\bin\glibmm-vc90-2_4.dll ..\..\install /Y
	copy ..\..\w64deps\gtkmm\bin\gtkmm-vc90-2_4.dll ..\..\install /Y
	copy ..\..\w64deps\gtkmm\bin\pangomm-vc90-1_4.dll ..\..\install /Y
	copy ..\..\w64deps\gtkmm\bin\sigc-vc90-2_0.dll ..\..\install /Y
	copy ..\..\w64deps\gtksourceviewmm\bin\gtksourceviewmm-vc90-2_2.dll ..\..\install /Y

GOTO GTKMM_END

:GTKMM_DEBUG
	@echo Copying GTKmm debug binaries
	copy ..\..\w64deps\gtkmm\bin\atkmm-vc90-d-1_6.dll ..\..\install /Y
	copy ..\..\w64deps\gtkmm\bin\cairomm-vc90-d-1_0.dll ..\..\install /Y
	copy ..\..\w64deps\gtkmm\bin\gdkmm-vc90-d-2_4.dll ..\..\install /Y
	copy ..\..\w64deps\gtkmm\bin\giomm-vc90-d-2_4.dll ..\..\install /Y
	copy ..\..\w64deps\gtkmm\bin\glibmm-vc90-d-2_4.dll ..\..\install /Y
	copy ..\..\w64deps\gtkmm\bin\gtkmm-vc90-d-2_4.dll ..\..\install /Y
	copy ..\..\w64deps\gtkmm\bin\pangomm-vc90-d-1_4.dll ..\..\install /Y
	copy ..\..\w64deps\gtkmm\bin\sigc-vc90-d-2_0.dll ..\..\install /Y
	copy ..\..\w64deps\gtksourceviewmm\bin\gtksourceviewmm-vc90-d-2_2.dll ..\..\install /Y

:GTKMM_END

@rem Copy the compiled GtkSourceView x64 DLLs to install
copy ..\..\build\libs\x64\libgtksourceview.dll ..\..\install /Y

@rem Copy the compiled GTKGlext x64 DLLs to install
copy ..\..\build\libs\x64\libgdkglext.dll ..\..\install /Y
copy ..\..\build\libs\x64\libgtkglext.dll ..\..\install /Y

@rem Copy the compiled libxml2 DLL to install
copy ..\..\build\libs\x64\libxml2.dll ..\..\install /Y