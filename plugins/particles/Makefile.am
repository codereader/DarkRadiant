AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/libs 

modulesdir = $(pkglibdir)/modules
modules_LTLIBRARIES = particles.la

particles_la_LDFLAGS = -module -avoid-version
particles_la_SOURCES = ParticlesManager.cpp particles.cpp ParticleStage.cpp

