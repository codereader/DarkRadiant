#pragma once

namespace render
{

class IGeometryStore;
class IRenderableObject;

// Helper object issuing the glDraw calls. Used by all kinds of render passes,
// be it Depth Fill, Interaction or Blend passes.
class ObjectRenderer
{
public:
    static void SubmitObject(IRenderableObject& object, IGeometryStore& store);
};

}
