if exist ..\..\install\lib rd ..\..\install\lib /S /Q
if exist ..\..\install\etc rd ..\..\install\etc /S /Q

del ..\..\install\*.dll

copy ..\..\w32deps\openal\bin\OpenAL32.dll ..\..\install /Y
copy ..\..\w32deps\openal\bin\wrap_oal.dll ..\..\install /Y

copy ..\..\w32deps\glew\bin\glew32.dll ..\..\install /Y

@rem copy ..\..\w32deps\libpng\bin\libpng13.dll ..\..\install /Y
@rem copy ..\..\w32deps\libiconv\bin\libiconv2.dll ..\..\install /Y

@rem md ..\..\install\etc
@rem xcopy ..\..\w32deps\gtk2\bin\etc\* ..\..\install\etc\. /S /Y

md ..\..\install\lib\gtk-2.0\engines
xcopy ..\..\w32deps\gtk2\bin\lib\gtk-2.0\engines\*.dll ..\..\install\lib\gtk-2.0\engines\. /S /Y

@rem Copy GTK+ libraries (32 bit)
IF "%1" == "Debug" GOTO GTK_DEBUG

	copy ..\..\w32deps\gtk2\bin\atk-vc100.dll ..\..\install /Y
	copy ..\..\w32deps\gtk2\bin\cairo-vc100.dll ..\..\install /Y
	copy ..\..\w32deps\gtk2\bin\cairo-gobject-vc100.dll ..\..\install /Y
	copy ..\..\w32deps\gtk2\bin\gdk-vc100.dll ..\..\install /Y
	copy ..\..\w32deps\gtk2\bin\gdk-pixbuf-vc100.dll ..\..\install /Y
	copy ..\..\w32deps\gtk2\bin\gio-vc100.dll ..\..\install /Y
	copy ..\..\w32deps\gtk2\bin\glib-vc100.dll ..\..\install /Y
	copy ..\..\w32deps\gtk2\bin\gmodule-vc100.dll ..\..\install /Y
	copy ..\..\w32deps\gtk2\bin\gobject-vc100.dll ..\..\install /Y
	copy ..\..\w32deps\gtk2\bin\gthread-vc100.dll ..\..\install /Y
	copy ..\..\w32deps\gtk2\bin\gtk-vc100.dll ..\..\install /Y
	copy ..\..\w32deps\gtk2\bin\pangocairo-vc100.dll ..\..\install /Y
	copy ..\..\w32deps\gtk2\bin\pango-vc100.dll ..\..\install /Y
	copy ..\..\w32deps\gtk2\bin\pangoft2-vc100.dll ..\..\install /Y
	copy ..\..\w32deps\gtk2\bin\pangowin32-vc100.dll ..\..\install /Y

	copy ..\..\w32deps\fontconfig\bin\fontconfig-vc100.dll ..\..\install /Y

	copy ..\..\w32deps\gettext\bin\gettext-vc100.dll ..\..\install /Y
	copy ..\..\w32deps\pixman\bin\pixman-vc100.dll ..\..\install /Y

GOTO GTK_END

:GTK_DEBUG

	copy ..\..\w32deps\gtk2\bin\atk-d-vc100.dll ..\..\install /Y
	copy ..\..\w32deps\gtk2\bin\cairo-d-vc100.dll ..\..\install /Y
	copy ..\..\w32deps\gtk2\bin\cairo-gobject-d-vc100.dll ..\..\install /Y
	copy ..\..\w32deps\gtk2\bin\gdk-d-vc100.dll ..\..\install /Y
	copy ..\..\w32deps\gtk2\bin\gdk-pixbuf-d-vc100.dll ..\..\install /Y
	copy ..\..\w32deps\gtk2\bin\gio-d-vc100.dll ..\..\install /Y
	copy ..\..\w32deps\gtk2\bin\glib-d-vc100.dll ..\..\install /Y
	copy ..\..\w32deps\gtk2\bin\gmodule-d-vc100.dll ..\..\install /Y
	copy ..\..\w32deps\gtk2\bin\gobject-d-vc100.dll ..\..\install /Y
	copy ..\..\w32deps\gtk2\bin\gthread-d-vc100.dll ..\..\install /Y
	copy ..\..\w32deps\gtk2\bin\gtk-d-vc100.dll ..\..\install /Y
	copy ..\..\w32deps\gtk2\bin\pangocairo-d-vc100.dll ..\..\install /Y
	copy ..\..\w32deps\gtk2\bin\pango-d-vc100.dll ..\..\install /Y
	copy ..\..\w32deps\gtk2\bin\pangoft2-d-vc100.dll ..\..\install /Y
	copy ..\..\w32deps\gtk2\bin\pangowin32-d-vc100.dll ..\..\install /Y

	copy ..\..\w32deps\fontconfig\bin\fontconfig-d-vc100.dll ..\..\install /Y

	copy ..\..\w32deps\gettext\bin\gettext-d-vc100.dll ..\..\install /Y
	copy ..\..\w32deps\pixman\bin\pixman-d-vc100.dll ..\..\install /Y

:GTK_END

copy ..\..\w32deps\python\bin\python26.dll ..\..\install /Y

@rem Copy gtkmm libraries (32 bit)
IF "%1" == "Debug" GOTO GTKMM_DEBUG

:GTKMM_RELEASE
	@echo Copying GTKmm release binaries
	copy ..\..\w32deps\gtkmm\bin\atkmm-vc100.dll ..\..\install /Y
	copy ..\..\w32deps\gtkmm\bin\cairomm-vc100.dll ..\..\install /Y
	copy ..\..\w32deps\gtkmm\bin\gdkmm-vc100.dll ..\..\install /Y
	copy ..\..\w32deps\gtkmm\bin\giomm-vc100.dll ..\..\install /Y
	copy ..\..\w32deps\gtkmm\bin\glibmm-vc100.dll ..\..\install /Y
	copy ..\..\w32deps\gtkmm\bin\gtkmm-vc100.dll ..\..\install /Y
	copy "..\..\w32deps\gtkmm\bin\libsigc++-vc100.dll" ..\..\install /Y
	copy ..\..\w32deps\gtkmm\bin\pangomm-vc100.dll ..\..\install /Y
	copy ..\..\w32deps\gtksourceviewmm\bin\gtksourceviewmm-vc100.dll ..\..\install /Y
	copy ..\..\w32deps\gtkglextmm\bin\gdkglextmm-vc100.dll ..\..\install /Y
	copy ..\..\w32deps\gtkglextmm\bin\gtkglextmm-vc100.dll ..\..\install /Y
GOTO GTKMM_END

:GTKMM_DEBUG
	@echo Copying GTKmm debug binaries
	copy ..\..\w32deps\gtkmm\bin\atkmm-d-vc100.dll ..\..\install /Y
	copy ..\..\w32deps\gtkmm\bin\cairomm-d-vc100.dll ..\..\install /Y
	copy ..\..\w32deps\gtkmm\bin\gdkmm-d-vc100.dll ..\..\install /Y
	copy ..\..\w32deps\gtkmm\bin\giomm-d-vc100.dll ..\..\install /Y
	copy ..\..\w32deps\gtkmm\bin\glibmm-d-vc100.dll ..\..\install /Y
	copy ..\..\w32deps\gtkmm\bin\gtkmm-d-vc100.dll ..\..\install /Y
	copy "..\..\w32deps\gtkmm\bin\libsigc++-d-vc100.dll" ..\..\install /Y
	copy ..\..\w32deps\gtkmm\bin\pangomm-d-vc100.dll ..\..\install /Y
	copy ..\..\w32deps\gtksourceviewmm\bin\gtksourceviewmm-d-vc100.dll ..\..\install /Y
	copy ..\..\w32deps\gtkglextmm\bin\gdkglextmm-d-vc100.dll ..\..\install /Y
	copy ..\..\w32deps\gtkglextmm\bin\gtkglextmm-d-vc100.dll ..\..\install /Y

:GTKMM_END

@rem Copy the compiled GtkSourceView x86 DLLs to install
IF "%1" == "Debug" GOTO GTKSOURCEVIEW_DEBUG

:GTKSOURCEVIEW_RELEASE
	@echo Copying gtksourceview release binaries
	copy ..\..\w32deps\gtksourceview-2.0\bin\gtksourceview-vc100.dll ..\..\install /Y

GOTO GTKSOURCEVIEW_END

:GTKSOURCEVIEW_DEBUG
	@echo Copying gtksourceview debug binaries
	copy ..\..\w32deps\gtksourceview-2.0\bin\gtksourceview-d-vc100.dll ..\..\install /Y

:GTKSOURCEVIEW_END

@rem Copy the GTKGlext x86 DLLs to install
IF "%1" == "Debug" GOTO GTKGLEXT_DEBUG

:GTKGLEXT_RELEASE
	@echo Copying GTKGlext release binaries
	copy ..\..\w32deps\gtkglext\bin\gdkglext-vc100.dll ..\..\install /Y
	copy ..\..\w32deps\gtkglext\bin\gtkglext-vc100.dll ..\..\install /Y

GOTO GTKGLEXT_END

:GTKGLEXT_DEBUG
	@echo Copying GTKGlext debug binaries
	copy ..\..\w32deps\gtkglext\bin\gdkglext-d-vc100.dll ..\..\install /Y
	copy ..\..\w32deps\gtkglext\bin\gtkglext-d-vc100.dll ..\..\install /Y

:GTKGLEXT_END

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
