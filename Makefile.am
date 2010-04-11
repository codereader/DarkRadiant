SUBDIRS = libs radiant plugins

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

# Install python scripts
scriptsdir = $(pkglibdir)/scripts
scripts_DATA = $(top_srcdir)/install/scripts/*.py
commandsdir = $(scriptsdir)/commands
commands_DATA = $(top_srcdir)/install/scripts/commands/*.py

# Install sourceviewer synax and colour files
sourceviewerdir = $(pkglibdir)/sourceviewer
sourceviewer_DATA = $(top_srcdir)/install/sourceviewer/*

# Install POT template
languagedir = $(pkglibdir)/i18n
language_DATA = $(top_srcdir)/install/i18n/*.pot

# Install German language module
languagededir = $(pkglibdir)/i18n/de/LC_MESSAGES
languagede_DATA = $(top_srcdir)/install/i18n/de/LC_MESSAGES/*.mo

