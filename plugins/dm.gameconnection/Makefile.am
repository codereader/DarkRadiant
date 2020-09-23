AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/libs

pluginsdir = $(pkglibdir)/plugins
plugins_LTLIBRARIES = dm_gameconnection.la

dm_gameconnection_la_LIBADD = $(top_builddir)/libs/wxutil/libwxutil.la \
      						  $(top_builddir)/libs/xmlutil/libxmlutil.la
dm_gameconnection_la_LDFLAGS = -module -avoid-version $(WX_LIBS) $(XML_LIBS)
dm_gameconnection_la_SOURCES = DiffDoom3MapWriter.cpp \
                               GameConnection.cpp \
                               MapObserver.cpp \
                               MessageTcp.cpp \
                               clsocket/ActiveSocket.cpp \
                               clsocket/PassiveSocket.cpp \
                               clsocket/SimpleSocket.cpp
