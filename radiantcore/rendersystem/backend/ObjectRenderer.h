#pragma once

#include <set>
#include "iobjectrenderer.h"

namespace render
{

class IGeometryStore;
class IRenderableObject;

// Helper object issuing the glDraw calls. Used by all kinds of render passes,
// be it Depth Fill, Interaction or Blend passes.
class ObjectRenderer :
    public IObjectRenderer
{
private:
    IGeometryStore& _store;

public:
    ObjectRenderer(IGeometryStore& store);

    // Initialise the vertex attribute pointers using the given start address (can be nullptr)
    void initAttributePointers() override;

    // Draws the given object, sets up transform and submits geometry
    void submitObject(IRenderableObject& object) override;

    // Draws the geometry of the given slot in the given primitive mode, no transforms
    void submitGeometry(IGeometryStore::Slot slot, GLenum primitiveMode) override;

    // Draws the specified number of instances of the geometry of the given slot in the given primitive mode, no transforms
    void submitInstancedGeometry(IGeometryStore::Slot slot, int numInstances, GLenum primitiveMode) override;

    // Draws the geometry with a custom set of indices
    void submitGeometryWithCustomIndices(IGeometryStore::Slot slot, GLenum primitiveMode, 
        const std::vector<unsigned int>& indices) override;

    // Draws all geometry as defined by their store IDs in the given mode, no transforms (std::set variant)
    void submitGeometry(const std::set<IGeometryStore::Slot>& slots, GLenum primitiveMode) override;
    // Draws all geometry as defined by their store IDs in the given mode, no transforms (std::vector variant)
    void submitGeometry(const std::vector<IGeometryStore::Slot>& slots, GLenum primitiveMode) override;

    // Draws all geometry as defined by their store IDs in the given mode, no transforms (std::vector variant)
    void submitInstancedGeometry(const std::vector<IGeometryStore::Slot>& slots, int numInstances, GLenum primitiveMode) override;
};

}
