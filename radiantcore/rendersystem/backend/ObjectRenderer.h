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
    // Draws the given object, sets up transform and submits geometry
    static void SubmitObject(IRenderableObject& object, IGeometryStore& store);

    // Draws the geometry of the given slot in the given primitive mode, no transforms
    static void SubmitGeometry(IGeometryStore::Slot slot, GLenum primitiveMode, IGeometryStore& store);
};

}
