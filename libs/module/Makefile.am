AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/libs 
AM_CXXFLAGS = -fPIC

pkglib_LTLIBRARIES = libmodule.la
libmodule_la_LDFLAGS = -release @PACKAGE_VERSION@
libmodule_la_SOURCES = CoreModule.cpp \
                     DynamicLibrary.cpp \
					 ApplicationContextBase.cpp \
					 StaticModule.cpp