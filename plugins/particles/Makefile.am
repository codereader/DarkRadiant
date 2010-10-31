AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/libs 

modulesdir = $(pkglibdir)/modules
modules_LTLIBRARIES = particles.la

particles_la_LDFLAGS = -module -avoid-version \
                       ${GL_LIBS}
particles_la_LIBADD = $(top_builddir)/libs/math/libmath.la
particles_la_SOURCES = ParticlesManager.cpp \
		       particles.cpp \
		       ParticleStage.cpp \
		       RenderableParticle.cpp \
		       RenderableParticleStage.cpp \
		       RenderableParticleBunch.cpp

