AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/libs 
AM_CXXFLAGS = -fPIC

pkglib_LTLIBRARIES = libmodules.la
libmodules_la_LDFLAGS = -release @PACKAGE_VERSION@
libmodules_la_SOURCES = CoreModule.cpp \
                     DynamicLibrary.cpp \
					 StaticModule.cpp