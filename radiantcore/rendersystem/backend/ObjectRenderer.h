#pragma once

#include <set>

namespace render
{

class IGeometryStore;
class IRenderableObject;

// Helper object issuing the glDraw calls. Used by all kinds of render passes,
// be it Depth Fill, Interaction or Blend passes.
class ObjectRenderer
{
public:
    // Initialise the vertex attribute pointers using the given start address (can be nullptr)
    static void InitAttributePointers(RenderVertex* bufferStart = nullptr);

    // Draws the given object, sets up transform and submits geometry
    static void SubmitObject(IRenderableObject& object, IGeometryStore& store);

    // Draws the geometry of the given slot in the given primitive mode, no transforms
    static void SubmitGeometry(IGeometryStore::Slot slot, GLenum primitiveMode, IGeometryStore& store);

    // Draws all geometry as defined by their store IDs in the given mode, no transforms (std::set variant)
    static void SubmitGeometry(const std::set<IGeometryStore::Slot>& slots, GLenum primitiveMode, IGeometryStore& store);
    // Draws all geometry as defined by their store IDs in the given mode, no transforms (std::vector variant)
    static void SubmitGeometry(const std::vector<IGeometryStore::Slot>& slots, GLenum primitiveMode, IGeometryStore& store);
};

}
