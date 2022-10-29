#include "ModelNodeBase.h"

namespace model
{

void ModelNodeBase::renderHighlights(IRenderableCollector& collector, const VolumeTest& volume)
{
    auto identity = Matrix4::getIdentity();

    for (const auto& surface : _renderableSurfaces)
    {
        collector.addHighlightRenderable(*surface, identity);
    }
}

}
