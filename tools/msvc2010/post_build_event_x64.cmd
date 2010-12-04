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
	copy ..\..\w64deps\gtkmm\bin\atkmm-vc100-1_6.dll ..\..\install /Y
	copy ..\..\w64deps\gtkmm\bin\cairomm-vc100-1_0.dll ..\..\install /Y
	copy ..\..\w64deps\gtkmm\bin\gdkmm-vc100-2_4.dll ..\..\install /Y
	copy ..\..\w64deps\gtkmm\bin\giomm-vc100-2_4.dll ..\..\install /Y
	copy ..\..\w64deps\gtkmm\bin\glibmm-vc100-2_4.dll ..\..\install /Y
	copy ..\..\w64deps\gtkmm\bin\gtkmm-vc100-2_4.dll ..\..\install /Y
	copy ..\..\w64deps\gtkmm\bin\pangomm-vc100-1_4.dll ..\..\install /Y
	copy ..\..\w64deps\gtkmm\bin\sigc-vc100-2_0.dll ..\..\install /Y
	copy ..\..\w64deps\gtksourceviewmm\bin\gtksourceviewmm-vc100-2_2.dll ..\..\install /Y
	copy ..\..\w64deps\gtkglextmm\bin\gdkglextmm-vc100-1_2.dll ..\..\install /Y
	copy ..\..\w64deps\gtkglextmm\bin\gtkglextmm-vc100-1_2.dll ..\..\install /Y

GOTO GTKMM_END

:GTKMM_DEBUG
	@echo Copying GTKmm debug binaries
	copy ..\..\w64deps\gtkmm\bin\atkmm-vc100-d-1_6.dll ..\..\install /Y
	copy ..\..\w64deps\gtkmm\bin\cairomm-vc100-d-1_0.dll ..\..\install /Y
	copy ..\..\w64deps\gtkmm\bin\gdkmm-vc100-d-2_4.dll ..\..\install /Y
	copy ..\..\w64deps\gtkmm\bin\giomm-vc100-d-2_4.dll ..\..\install /Y
	copy ..\..\w64deps\gtkmm\bin\glibmm-vc100-d-2_4.dll ..\..\install /Y
	copy ..\..\w64deps\gtkmm\bin\gtkmm-vc100-d-2_4.dll ..\..\install /Y
	copy ..\..\w64deps\gtkmm\bin\pangomm-vc100-d-1_4.dll ..\..\install /Y
	copy ..\..\w64deps\gtkmm\bin\sigc-vc100-d-2_0.dll ..\..\install /Y
	copy ..\..\w64deps\gtksourceviewmm\bin\gtksourceviewmm-vc100-d-2_2.dll ..\..\install /Y
	copy ..\..\w64deps\gtkglextmm\bin\gdkglextmm-vc100-d-1_2.dll ..\..\install /Y
	copy ..\..\w64deps\gtkglextmm\bin\gtkglextmm-vc100-d-1_2.dll ..\..\install /Y

:GTKMM_END

@rem Copy the GtkSourceView x64 DLL to install
IF "%1" == "Debug" GOTO GTKSOURCEVIEW_DEBUG

:GTKSOURCEVIEW_RELEASE
	@echo Copying gtksourceview release binaries
	copy ..\..\w64deps\gtksourceview-2.0\bin\libgtksourceview-vc100-2_0.dll ..\..\install /Y

GOTO GTKSOURCEVIEW_END

:GTKSOURCEVIEW_DEBUG
	@echo Copying gtksourceview debug binaries
	copy ..\..\w64deps\gtksourceview-2.0\bin\libgtksourceview-vc100-d-2_0.dll ..\..\install /Y

:GTKSOURCEVIEW_END

@rem Copy the GTKGlext x64 DLLs to install
IF "%1" == "Debug" GOTO GTKGLEXT_DEBUG

:GTKGLEXT_RELEASE
	@echo Copying GTKGlext release binaries
	copy ..\..\w64deps\gtkglext\bin\libgdkglext-vc100-1_2.dll ..\..\install /Y
	copy ..\..\w64deps\gtkglext\bin\libgtkglext-vc100-1_2.dll ..\..\install /Y

GOTO GTKGLEXT_END

:GTKGLEXT_DEBUG
	@echo Copying GTKGlext debug binaries
	copy ..\..\w64deps\gtkglext\bin\libgdkglext-vc100-d-1_2.dll ..\..\install /Y
	copy ..\..\w64deps\gtkglext\bin\libgtkglext-vc100-d-1_2.dll ..\..\install /Y

:GTKGLEXT_END

@rem Copy the libxml2 x86 DLL to install
IF "%1" == "Debug" GOTO LIBXML2_DEBUG

:LIBXML2_RELEASE
	@echo Copying libxml2 release binaries
	copy ..\..\w64deps\libxml2\bin\libxml2-vc100.dll ..\..\install /Y

GOTO LIBXML2_END

:LIBXML2_DEBUG
	@echo Copying libxml2 debug binaries
	copy ..\..\w64deps\libxml2\bin\libxml2-d-vc100.dll ..\..\install /Y

:LIBXML2_END
