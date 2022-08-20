#include "BlendLight.h"

namespace render
{

BlendLight::BlendLight(RendererLight& light, IGeometryStore& store, IObjectRenderer& objectRenderer) :
    _light(light),
    _store(store),
    _objectRenderer(objectRenderer),
    _lightBounds(light.lightAABB())
{}

}
