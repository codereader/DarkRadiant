AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/libs \
			  $(GTK_CFLAGS) $(XML_CFLAGS)

pluginsdir = $(pkglibdir)/plugins
plugins_LTLIBRARIES = dm_conversation.la

dm_conversation_la_LIBADD = $(top_builddir)/libs/gtkutil/libgtkutil.la \
						    $(top_builddir)/libs/xmlutil/libxmlutil.la
dm_conversation_la_LDFLAGS = -module -avoid-version \
							 $(GTK_LIBS) $(XML_LIBS) $(REGEX_LIBS)
dm_conversation_la_SOURCES = plugin.cpp \
							 ConversationKeyExtractor.cpp \
							 ConversationCommandInfo.cpp \
							 CommandArgumentItem.cpp \
							 ConversationEntity.cpp \
							 CommandEditor.cpp \
							 ConversationEditor.cpp \
							 ConversationCommand.cpp \
							 ConversationCommandLibrary.cpp \
							 ConversationDialog.cpp
