if exist ..\..\install\lib rd ..\..\install\lib /S /Q
if exist ..\..\install\etc rd ..\..\install\etc /S /Q

del ..\..\install\*.dll

copy ..\..\w32deps\openal\bin\OpenAL32.dll ..\..\install /Y
copy ..\..\w32deps\openal\bin\wrap_oal.dll ..\..\install /Y

copy ..\..\w32deps\glew\bin\glew32.dll ..\..\install /Y

@rem copy ..\..\w32deps\libpng\bin\libpng13.dll ..\..\install /Y
@rem copy ..\..\w32deps\libiconv\bin\libiconv2.dll ..\..\install /Y

md ..\..\install\etc
xcopy ..\..\w32deps\gtk2\bin\etc\* ..\..\install\etc\. /S /Y

md ..\..\install\lib\gtk-2.0\engines

@rem Copy GTK+ libraries (32 bit)
IF "%1" == "Debug" GOTO GTK_DEBUG

	xcopy ..\..\w32deps\gtk2\bin\lib\gtk-2.0\engines\*.dll ..\..\install\lib\gtk-2.0\engines\. /S /Y

	copy ..\..\w32deps\gtk2\bin\atk-%2.dll ..\..\install /Y
	copy ..\..\w32deps\gtk2\bin\cairo-%2.dll ..\..\install /Y
	copy ..\..\w32deps\gtk2\bin\cairo-gobject-%2.dll ..\..\install /Y
	copy ..\..\w32deps\gtk2\bin\gdk-%2.dll ..\..\install /Y
	copy ..\..\w32deps\gtk2\bin\gdk-pixbuf-%2.dll ..\..\install /Y
	copy ..\..\w32deps\gtk2\bin\gio-%2.dll ..\..\install /Y
	copy ..\..\w32deps\gtk2\bin\glib-%2.dll ..\..\install /Y
	copy ..\..\w32deps\gtk2\bin\gmodule-%2.dll ..\..\install /Y
	copy ..\..\w32deps\gtk2\bin\gobject-%2.dll ..\..\install /Y
	copy ..\..\w32deps\gtk2\bin\gthread-%2.dll ..\..\install /Y
	copy ..\..\w32deps\gtk2\bin\gtk-%2.dll ..\..\install /Y
	copy ..\..\w32deps\gtk2\bin\pangocairo-%2.dll ..\..\install /Y
	copy ..\..\w32deps\gtk2\bin\pango-%2.dll ..\..\install /Y
	copy ..\..\w32deps\gtk2\bin\pangoft2-%2.dll ..\..\install /Y
	copy ..\..\w32deps\gtk2\bin\pangowin32-%2.dll ..\..\install /Y

	copy ..\..\w32deps\fontconfig\bin\fontconfig-%2.dll ..\..\install /Y
	copy ..\..\w32deps\ftgl\bin\ftgl-%2.dll ..\..\install /Y

	copy ..\..\w32deps\gettext\bin\gettext-%2.dll ..\..\install /Y
	copy ..\..\w32deps\pixman\bin\pixman-%2.dll ..\..\install /Y

GOTO GTK_END

:GTK_DEBUG

	@rem Use the debug version of the libwimp.dll
	xcopy ..\..\w32deps\gtk2\bin\lib\gtk-2.0\engines\debug\*.dll ..\..\install\lib\gtk-2.0\engines\. /S /Y

	copy ..\..\w32deps\gtk2\bin\atk-d-%2.dll ..\..\install /Y
	copy ..\..\w32deps\gtk2\bin\cairo-d-%2.dll ..\..\install /Y
	copy ..\..\w32deps\gtk2\bin\cairo-gobject-d-%2.dll ..\..\install /Y
	copy ..\..\w32deps\gtk2\bin\gdk-d-%2.dll ..\..\install /Y
	copy ..\..\w32deps\gtk2\bin\gdk-pixbuf-d-%2.dll ..\..\install /Y
	copy ..\..\w32deps\gtk2\bin\gio-d-%2.dll ..\..\install /Y
	copy ..\..\w32deps\gtk2\bin\glib-d-%2.dll ..\..\install /Y
	copy ..\..\w32deps\gtk2\bin\gmodule-d-%2.dll ..\..\install /Y
	copy ..\..\w32deps\gtk2\bin\gobject-d-%2.dll ..\..\install /Y
	copy ..\..\w32deps\gtk2\bin\gthread-d-%2.dll ..\..\install /Y
	copy ..\..\w32deps\gtk2\bin\gtk-d-%2.dll ..\..\install /Y
	copy ..\..\w32deps\gtk2\bin\pangocairo-d-%2.dll ..\..\install /Y
	copy ..\..\w32deps\gtk2\bin\pango-d-%2.dll ..\..\install /Y
	copy ..\..\w32deps\gtk2\bin\pangoft2-d-%2.dll ..\..\install /Y
	copy ..\..\w32deps\gtk2\bin\pangowin32-d-%2.dll ..\..\install /Y

	copy ..\..\w32deps\fontconfig\bin\fontconfig-d-%2.dll ..\..\install /Y
	copy ..\..\w32deps\ftgl\bin\ftgl-d-%2.dll ..\..\install /Y

	copy ..\..\w32deps\gettext\bin\gettext-d-%2.dll ..\..\install /Y
	copy ..\..\w32deps\pixman\bin\pixman-d-%2.dll ..\..\install /Y

:GTK_END

copy ..\..\w32deps\python\bin\python26.dll ..\..\install /Y

@rem Copy gtkmm libraries (32 bit)
IF "%1" == "Debug" GOTO GTKMM_DEBUG

:GTKMM_RELEASE
	@echo Copying GTKmm release binaries
	copy ..\..\w32deps\gtkmm\bin\atkmm-%2.dll ..\..\install /Y
	copy ..\..\w32deps\gtkmm\bin\cairomm-%2.dll ..\..\install /Y
	copy ..\..\w32deps\gtkmm\bin\gdkmm-%2.dll ..\..\install /Y
	copy ..\..\w32deps\gtkmm\bin\giomm-%2.dll ..\..\install /Y
	copy ..\..\w32deps\gtkmm\bin\glibmm-%2.dll ..\..\install /Y
	copy ..\..\w32deps\gtkmm\bin\gtkmm-%2.dll ..\..\install /Y
	copy "..\..\w32deps\gtkmm\bin\libsigc++-%2.dll" ..\..\install /Y
	copy ..\..\w32deps\gtkmm\bin\pangomm-%2.dll ..\..\install /Y
	copy ..\..\w32deps\gtksourceviewmm\bin\gtksourceviewmm-%2.dll ..\..\install /Y
	copy ..\..\w32deps\gtkglextmm\bin\gdkglextmm-%2.dll ..\..\install /Y
	copy ..\..\w32deps\gtkglextmm\bin\gtkglextmm-%2.dll ..\..\install /Y
GOTO GTKMM_END

:GTKMM_DEBUG
	@echo Copying GTKmm debug binaries
	copy ..\..\w32deps\gtkmm\bin\atkmm-d-%2.dll ..\..\install /Y
	copy ..\..\w32deps\gtkmm\bin\cairomm-d-%2.dll ..\..\install /Y
	copy ..\..\w32deps\gtkmm\bin\gdkmm-d-%2.dll ..\..\install /Y
	copy ..\..\w32deps\gtkmm\bin\giomm-d-%2.dll ..\..\install /Y
	copy ..\..\w32deps\gtkmm\bin\glibmm-d-%2.dll ..\..\install /Y
	copy ..\..\w32deps\gtkmm\bin\gtkmm-d-%2.dll ..\..\install /Y
	copy "..\..\w32deps\gtkmm\bin\libsigc++-d-%2.dll" ..\..\install /Y
	copy ..\..\w32deps\gtkmm\bin\pangomm-d-%2.dll ..\..\install /Y
	copy ..\..\w32deps\gtksourceviewmm\bin\gtksourceviewmm-d-%2.dll ..\..\install /Y
	copy ..\..\w32deps\gtkglextmm\bin\gdkglextmm-d-%2.dll ..\..\install /Y
	copy ..\..\w32deps\gtkglextmm\bin\gtkglextmm-d-%2.dll ..\..\install /Y

:GTKMM_END

@rem Copy the compiled GtkSourceView x86 DLLs to install
IF "%1" == "Debug" GOTO GTKSOURCEVIEW_DEBUG

:GTKSOURCEVIEW_RELEASE
	@echo Copying gtksourceview release binaries
	copy ..\..\w32deps\gtksourceview-2.0\bin\gtksourceview-%2.dll ..\..\install /Y

GOTO GTKSOURCEVIEW_END

:GTKSOURCEVIEW_DEBUG
	@echo Copying gtksourceview debug binaries
	copy ..\..\w32deps\gtksourceview-2.0\bin\gtksourceview-d-%2.dll ..\..\install /Y

:GTKSOURCEVIEW_END

@rem Copy the GTKGlext x86 DLLs to install
IF "%1" == "Debug" GOTO GTKGLEXT_DEBUG

:GTKGLEXT_RELEASE
	@echo Copying GTKGlext release binaries
	copy ..\..\w32deps\gtkglext\bin\gdkglext-%2.dll ..\..\install /Y
	copy ..\..\w32deps\gtkglext\bin\gtkglext-%2.dll ..\..\install /Y

GOTO GTKGLEXT_END

:GTKGLEXT_DEBUG
	@echo Copying GTKGlext debug binaries
	copy ..\..\w32deps\gtkglext\bin\gdkglext-d-%2.dll ..\..\install /Y
	copy ..\..\w32deps\gtkglext\bin\gtkglext-d-%2.dll ..\..\install /Y

:GTKGLEXT_END

@rem Copy the libxml2 x86 DLL to install
IF "%1" == "Debug" GOTO LIBXML2_DEBUG

:LIBXML2_RELEASE
	@echo Copying libxml2 release binaries
	copy ..\..\w32deps\libxml2\bin\libxml2-%2.dll ..\..\install /Y

GOTO LIBXML2_END

:LIBXML2_DEBUG
	@echo Copying libxml2 debug binaries
	copy ..\..\w32deps\libxml2\bin\libxml2-d-%2.dll ..\..\install /Y

:LIBXML2_END
