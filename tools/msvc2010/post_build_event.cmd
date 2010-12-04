if exist ..\..\install\lib rd ..\..\install\lib /S /Q
if exist ..\..\install\etc rd ..\..\install\etc /S /Q

del ..\..\install\*.dll

copy ..\..\w32deps\openal\bin\OpenAL32.dll ..\..\install /Y
copy ..\..\w32deps\openal\bin\wrap_oal.dll ..\..\install /Y

copy ..\..\w32deps\glew\bin\glew32.dll ..\..\install /Y

copy ..\..\w32deps\libpng\bin\libpng13.dll ..\..\install /Y
copy ..\..\w32deps\libiconv\bin\libiconv2.dll ..\..\install /Y

md ..\..\install\etc
xcopy ..\..\w32deps\gtk2\bin\etc\* ..\..\install\etc\. /S /Y

md ..\..\install\lib
xcopy ..\..\w32deps\gtk2\bin\lib\* ..\..\install\lib\. /S /Y

@rem copy ..\..\w32deps\gtk2\bin\iconv.dll ..\..\install /Y
copy ..\..\w32deps\gtk2\bin\intl.dll ..\..\install /Y
copy ..\..\w32deps\gtk2\bin\jpeg62.dll ..\..\install /Y
copy ..\..\w32deps\gtk2\bin\libatk-1.0-0.dll ..\..\install /Y
copy ..\..\w32deps\gtk2\bin\libgailutil-18.dll ..\..\install /Y
copy ..\..\w32deps\gtk2\bin\libcairo-2.dll ..\..\install /Y
@rem copy ..\..\w32deps\gtk2\bin\libfontconfig-1.dll ..\..\install /Y
copy ..\..\w32deps\gtk2\bin\libgdk_pixbuf-2.0-0.dll ..\..\install /Y
copy ..\..\w32deps\gtk2\bin\libgdk-win32-2.0-0.dll ..\..\install /Y
copy ..\..\w32deps\gtk2\bin\libgio-2.0-0.dll ..\..\install /Y
copy ..\..\w32deps\gtk2\bin\libglib-2.0-0.dll ..\..\install /Y
copy ..\..\w32deps\gtk2\bin\libgmodule-2.0-0.dll ..\..\install /Y
copy ..\..\w32deps\gtk2\bin\libgobject-2.0-0.dll ..\..\install /Y
copy ..\..\w32deps\gtk2\bin\libgthread-2.0-0.dll ..\..\install /Y
copy ..\..\w32deps\gtk2\bin\libgtk-win32-2.0-0.dll ..\..\install /Y
copy ..\..\w32deps\gtk2\bin\libpango-1.0-0.dll ..\..\install /Y
copy ..\..\w32deps\gtk2\bin\libpangocairo-1.0-0.dll ..\..\install /Y
copy ..\..\w32deps\gtk2\bin\libpangoft2-1.0-0.dll ..\..\install  /Y
copy ..\..\w32deps\gtk2\bin\libpangowin32-1.0-0.dll ..\..\install /Y
copy ..\..\w32deps\gtk2\bin\libpng12-0.dll ..\..\install /Y
copy ..\..\w32deps\gtk2\bin\libtiff3.dll ..\..\install /Y
copy ..\..\w32deps\gtk2\bin\zlib1.dll ..\..\install /Y

copy ..\..\w32deps\python\bin\python27.dll ..\..\install /Y

@rem Copy gtkmm libraries (32 bit)
IF "%1" == "Debug" GOTO GTKMM_DEBUG

:GTKMM_RELEASE
	@echo Copying GTKmm release binaries
	copy ..\..\w32deps\gtkmm\bin\atkmm-vc100-1_6.dll ..\..\install /Y
	copy ..\..\w32deps\gtkmm\bin\cairomm-vc100-1_0.dll ..\..\install /Y
	copy ..\..\w32deps\gtkmm\bin\gdkmm-vc100-2_4.dll ..\..\install /Y
	copy ..\..\w32deps\gtkmm\bin\giomm-vc100-2_4.dll ..\..\install /Y
	copy ..\..\w32deps\gtkmm\bin\glibmm-vc100-2_4.dll ..\..\install /Y
	copy ..\..\w32deps\gtkmm\bin\gtkmm-vc100-2_4.dll ..\..\install /Y
	copy ..\..\w32deps\gtkmm\bin\pangomm-vc100-1_4.dll ..\..\install /Y
	copy ..\..\w32deps\gtkmm\bin\sigc-vc100-2_0.dll ..\..\install /Y
	copy ..\..\w32deps\gtksourceviewmm\bin\gtksourceviewmm-vc100-2_2.dll ..\..\install /Y
	copy ..\..\w32deps\gtkglextmm\bin\gdkglextmm-vc100-1_2.dll ..\..\install /Y
	copy ..\..\w32deps\gtkglextmm\bin\gtkglextmm-vc100-1_2.dll ..\..\install /Y

GOTO GTKMM_END

:GTKMM_DEBUG
	@echo Copying GTKmm debug binaries
	copy ..\..\w32deps\gtkmm\bin\atkmm-vc100-d-1_6.dll ..\..\install /Y
	copy ..\..\w32deps\gtkmm\bin\cairomm-vc100-d-1_0.dll ..\..\install /Y
	copy ..\..\w32deps\gtkmm\bin\gdkmm-vc100-d-2_4.dll ..\..\install /Y
	copy ..\..\w32deps\gtkmm\bin\giomm-vc100-d-2_4.dll ..\..\install /Y
	copy ..\..\w32deps\gtkmm\bin\glibmm-vc100-d-2_4.dll ..\..\install /Y
	copy ..\..\w32deps\gtkmm\bin\gtkmm-vc100-d-2_4.dll ..\..\install /Y
	copy ..\..\w32deps\gtkmm\bin\pangomm-vc100-d-1_4.dll ..\..\install /Y
	copy ..\..\w32deps\gtkmm\bin\sigc-vc100-d-2_0.dll ..\..\install /Y
	copy ..\..\w32deps\gtksourceviewmm\bin\gtksourceviewmm-vc100-d-2_2.dll ..\..\install /Y
	copy ..\..\w32deps\gtkglextmm\bin\gdkglextmm-vc100-d-1_2.dll ..\..\install /Y
	copy ..\..\w32deps\gtkglextmm\bin\gtkglextmm-vc100-d-1_2.dll ..\..\install /Y

:GTKMM_END

@rem Copy the compiled GtkSourceView x86 DLLs to install
copy ..\..\w32deps\gtksourceview-2.0\bin\libgtksourceview.dll ..\..\install /Y

@rem Copy the GTKGlext x86 DLLs to install
copy ..\..\w32deps\gtkglext\bin\libgdkglext.dll ..\..\install /Y
copy ..\..\w32deps\gtkglext\bin\libgtkglext.dll ..\..\install /Y

@rem Copy the libxml2 x86 DLL to install
IF "%1" == "Debug" GOTO LIBXML2_DEBUG

:LIBXML2_RELEASE
	@echo Copying libxml2 release binaries
	copy ..\..\w32deps\libxml2\bin\libxml2-vc100.dll ..\..\install /Y

GOTO LIBXML2_END

:LIBXML2_DEBUG
	@echo Copying libxml2 debug binaries
	copy ..\..\w32deps\libxml2\bin\libxml2-d-vc100.dll ..\..\install /Y

:LIBXML2_END
