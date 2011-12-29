ACLOCAL_AMFLAGS = -I m4

SUBDIRS = libs radiant plugins install/i18n

# Install data and stuff
gamedir = $(pkgdatadir)/games
game_DATA = $(top_srcdir)/install/games/*.game 

bitmapsdir = $(pkgdatadir)/bitmaps
bitmaps_DATA = $(top_srcdir)/install/bitmaps/*

applicationsdir = $(datadir)/applications
applications_DATA = $(top_builddir)/install/darkradiant.desktop

# Install XML config files
pkgdata_DATA = $(top_srcdir)/install/colours.xml \
               $(top_srcdir)/install/commandsystem.xml \
			   $(top_srcdir)/install/user.xml \
			   $(top_srcdir)/install/menu.xml \
			   $(top_srcdir)/install/input.xml \
			   $(top_srcdir)/install/debug.xml

# Install GL programs
glprogsdir = $(pkgdatadir)/gl
glprogs_DATA = $(top_srcdir)/install/gl/*

# Install fonts
fontsdir = $(pkgdatadir)/ui/fonts
fonts_DATA = $(top_srcdir)/install/ui/fonts/*

# Install UI files
uifilesdir = $(pkgdatadir)/ui
uifiles_DATA = $(top_srcdir)/install/ui/*.glade

# Install python scripts
scriptsdir = $(pkglibdir)/scripts
scripts_DATA = $(top_srcdir)/install/scripts/*.py
commandsdir = $(scriptsdir)/commands
commands_DATA = $(top_srcdir)/install/scripts/commands/*.py

# Install sourceviewer synax and colour files
sourceviewerdir = $(pkglibdir)/sourceviewer
sourceviewer_DATA = $(top_srcdir)/install/sourceviewer/*

EXTRA_DIST = config.rpath
