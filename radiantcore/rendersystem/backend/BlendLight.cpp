#include "BlendLight.h"

namespace render
{

BlendLight::BlendLight(RendererLight& light, IGeometryStore& store, IObjectRenderer& objectRenderer) :
    _light(light),
    _store(store),
    _objectRenderer(objectRenderer),
    _lightBounds(light.lightAABB())
{}

bool BlendLight::isInView(const IRenderView& view)
{
    return view.TestAABB(_lightBounds) != VOLUME_OUTSIDE;
}

void BlendLight::collectSurfaces(const IRenderView& view, const std::set<IRenderEntityPtr>& entities)
{
    // TODO
}

}
