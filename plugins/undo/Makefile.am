AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/libs $(XML_CFLAGS)

modulesdir = $(pkglibdir)/modules
modules_LTLIBRARIES = undo.la

undo_la_LDFLAGS = -module -avoid-version
undo_la_SOURCES = UndoSystem.cpp

