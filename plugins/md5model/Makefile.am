AM_CPPFLAGS = -I../../include -I../../libs 

pkglib_LTLIBRARIES = md5model.la
md5model_la_LDFLAGS = -module -avoid-version
md5model_la_SOURCES = MD5Model.cpp \
                      MD5ModelNode.cpp \
                      MD5Surface.cpp \
                      plugin.cpp \
                      MD5ModelLoader.cpp

# Override the Automake install rule, we want to install in $(pkglibdir)/modules
# not just $(pkglibdir).
install-exec-am:
	mkdir -p "$(DESTDIR)$(pkglibdir)/modules" && \
	$(INSTALL) .libs/md5model.so "$(DESTDIR)$(pkglibdir)/modules"

