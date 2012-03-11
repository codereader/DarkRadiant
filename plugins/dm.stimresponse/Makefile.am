AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/libs \
               $(XML_CFLAGS) $(GTKMM_CFLAGS)

pluginsdir = $(pkglibdir)/plugins
plugins_LTLIBRARIES = dm_stimresponse.la

dm_stimresponse_la_LIBADD = $(top_builddir)/libs/xmlutil/libxmlutil.la \
                            $(top_builddir)/libs/gtkutil/libgtkutil.la
dm_stimresponse_la_LDFLAGS = -module -avoid-version \
                             $(BOOST_REGEX_LIBS) $(GTKMM_LIBS) $(XML_LIBS)
dm_stimresponse_la_SOURCES = ResponseEditor.cpp \
                      plugin.cpp \
                      StimResponse.cpp \
                      EffectArgumentItem.cpp \
                      SRPropertyRemover.cpp \
                      StimResponseEditor.cpp \
                      SREntity.cpp \
                      CustomStimEditor.cpp \
                      EffectEditor.cpp \
                      SRPropertyLoader.cpp \
                      ClassEditor.cpp \
                      ResponseEffect.cpp \
                      SRPropertySaver.cpp \
                      StimTypes.cpp \
                      ResponseEffectTypes.cpp \
                      StimEditor.cpp
