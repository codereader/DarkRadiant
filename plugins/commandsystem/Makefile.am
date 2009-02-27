AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/libs \
			  $(XML_CFLAGS)

modulesdir = $(pkglibdir)/modules
modules_LTLIBRARIES = commandsystem.la

commandsystem_la_LDFLAGS = -module -avoid-version 
commandsystem_la_SOURCES = CommandSystem.cpp
