AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/libs

pkglib_LTLIBRARIES = libscenegraph.la
libscenegraph_la_LDFLAGS = -release @PACKAGE_VERSION@
libscenegraph_la_SOURCES = InstanceWalkers.cpp \
			 			  TraversableNodeSet.cpp \
						  Node.cpp \
                          ../UndoableObject.cpp
