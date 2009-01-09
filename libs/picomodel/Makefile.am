AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/libs 
AM_CFLAGS = -fPIC

pkglib_LTLIBRARIES = libpicomodel.la
libpicomodel_la_LDFLAGS = -release @PACKAGE_VERSION@ -lm
libpicomodel_la_SOURCES = picointernal.c \
                         lwo/vecmath.c \
                         lwo/clip.c \
                         lwo/surface.c \
                         lwo/pntspols.c \
                         lwo/list.c \
                         lwo/envelope.c \
                         lwo/vmap.c \
                         lwo/lwio.c \
                         lwo/lwo2.c \
                         lwo/lwob.c \
                         pm_mdc.c \
                         pm_terrain.c \
                         pm_3ds.c \
                         pm_md3.c \
                         picomodel.c \
                         pm_ms3d.c \
                         pm_obj.c \
                         pm_lwo.c \
                         pm_fm.c \
                         pm_md2.c \
                         pm_ase.c \
                         picomodules.c
