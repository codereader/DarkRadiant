#pragma once

#include <set>
#include <vector>
#include "igl.h"
#include "igeometrystore.h"

namespace render
{

class IRenderableObject;

/**
 * An IObjectRenderer issues the openGL draw calls to render
 * the specified geometry in the attached IGeometryStore.
 */
class IObjectRenderer
{
public:
    virtual ~IObjectRenderer() {}

    // Sets up the renderer. Must be called before submitting any geometry
    virtual void initAttributePointers() = 0;

    // Draws the given object, sets up transform and submits geometry
    virtual void submitObject(IRenderableObject& object) = 0;

    // Draws the geometry of the given slot in the given primitive mode, no transforms
    virtual void submitGeometry(IGeometryStore::Slot slot, GLenum primitiveMode) = 0;

    // Draws the specified number of instances of the geometry of the given slot in the given primitive mode, no transforms
    virtual void submitInstancedGeometry(IGeometryStore::Slot slot, int numInstances, GLenum primitiveMode) = 0;

    // Draws all geometry as defined by their store IDs in the given mode, no transforms (std::set variant)
    virtual void submitGeometry(const std::set<IGeometryStore::Slot>& slots, GLenum primitiveMode) = 0;

    // Draws all geometry as defined by their store IDs in the given mode, no transforms (std::vector variant)
    virtual void submitGeometry(const std::vector<IGeometryStore::Slot>& slots, GLenum primitiveMode) = 0;

    // Draws all geometry as defined by their store IDs in the given mode, no transforms (std::vector variant)
    virtual void submitInstancedGeometry(const std::vector<IGeometryStore::Slot>& slots, int numInstances, GLenum primitiveMode) = 0;

    // Draws the geometry with a custom set of indices
    virtual void submitGeometryWithCustomIndices(IGeometryStore::Slot slot, GLenum primitiveMode,
        const std::vector<unsigned int>& indices) = 0;
};

}
