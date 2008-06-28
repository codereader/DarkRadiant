libspath = ../../libs
AM_CPPFLAGS = -I../../include -I$(libspath) $(XML_CFLAGS)

modulesdir = $(pkglibdir)/modules
modules_LTLIBRARIES = entity.la

entity_la_LDFLAGS = -module -avoid-version $(GL_LIBS)
entity_la_LIBADD = $(libspath)/math/libmath.a $(libspath)/scene/libscenegraph.a
entity_la_SOURCES = origin.cpp \
                    angle.cpp \
                    ModelKey.cpp \
                    rotation.cpp \
                    light/Light.cpp \
                    light/LightSettings.cpp \
                    light/Renderables.cpp \
                    light/LightNode.cpp \
                    eclassmodel/EclassModel.cpp \
                    eclassmodel/EclassModelNode.cpp \
                    EntityCreator.cpp \
                    namedentity.cpp \
                    speaker/Speaker.cpp \
                    speaker/SpeakerRenderables.cpp \
                    speaker/SpeakerSettings.cpp \
                    speaker/SpeakerNode.cpp \
                    Doom3Entity.cpp \
                    EntityNode.cpp \
                    doom3group/Doom3Group.cpp \
                    doom3group/Doom3GroupNode.cpp \
                    angles.cpp \
                    curve/CurveCatmullRom.cpp \
                    curve/CurveNURBS.cpp \
                    curve/Curve.cpp \
                    curve/CurveEditInstance.cpp \
                    plugin.cpp \
                    generic/GenericEntity.cpp \
                    generic/GenericEntityNode.cpp \
                    keyobservers.cpp \
                    NamespaceManager.cpp \
                    KeyValueObserver.cpp \
                    NameKeyObserver.cpp \
                    colour.cpp \
                    entity.cpp \
                    scale.cpp \
                    KeyValue.cpp \
                    target/TargetKey.cpp \
                    target/TargetableNode.cpp \
                    target/TargetKeyCollection.cpp \
                    target/TargetManager.cpp \
                    target/RenderableTargetInstances.cpp

