AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/libs $(GTKMM_CFLAGS)

modulesdir = $(pkglibdir)/modules
modules_LTLIBRARIES = particles.la

particles_la_LDFLAGS = -module -avoid-version \
                       $(GL_LIBS) \
                       $(GTKMM_LIBS) \
                       $(BOOST_FILESYSTEM_LIBS) \
                       $(BOOST_REGEX_LIBS) \
                       $(BOOST_SYSTEM_LIBS)
particles_la_LIBADD = $(top_builddir)/libs/math/libmath.la \
                      $(top_builddir)/libs/scene/libscenegraph.la \
                      $(top_builddir)/libs/gtkutil/libgtkutil.la
particles_la_SOURCES = ParticlesManager.cpp \
                       particles.cpp \
                       ParticleDef.cpp \
                       ParticleNode.cpp \
                       ParticleParameter.cpp \
                       StageDef.cpp \
                       RenderableParticle.cpp \
                       RenderableParticleStage.cpp \
                       RenderableParticleBunch.cpp \
                       editor/ParticleEditor.cpp

