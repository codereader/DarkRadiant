AM_CPPFLAGS = -I../../include -I../../libs $(XML_CFLAGS)

pkglib_LTLIBRARIES = entity.la
entity_la_LDFLAGS = -module -avoid-version
entity_la_LIBADD = ../../libs/math/libmath.a
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
                    namekeys.cpp \
                    colour.cpp \
                    entity.cpp \
                    scale.cpp \
                    KeyValue.cpp \
                    target/TargetKey.cpp \
                    target/TargetableNode.cpp \
                    target/TargetKeyCollection.cpp \
                    target/TargetManager.cpp \
                    target/RenderableTargetInstances.cpp

# Override the Automake install rule, we want to install in $(pkglibdir)/modules
# not just $(pkglibdir).
install-exec-am:
	mkdir -p "$(DESTDIR)$(pkglibdir)/modules" && \
	$(INSTALL) .libs/entity.so "$(DESTDIR)$(pkglibdir)/modules"

