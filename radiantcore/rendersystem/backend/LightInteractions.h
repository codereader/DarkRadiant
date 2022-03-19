#pragma once

#include <map>
#include <vector>
#include <set>
#include "irender.h"
#include "isurfacerenderer.h"
#include "irenderableobject.h"
#include "irenderview.h"

namespace render
{

class OpenGLState;
class OpenGLShader;
class GLSLDepthFillAlphaProgram;
class GLSLBumpProgram;

/**
 * Defines interactions between a light and one or more entity renderables
 * It only lives through the course of a single render pass, therefore direct
 * references without ref-counting are used.
 * 
 * Objects are grouped by entity, then by shader.
 */
class LightInteractions
{
private:
    RendererLight& _light;
    IGeometryStore& _store;
    AABB _lightBounds;

    // A flat list of renderables
    using ObjectList = std::vector<std::reference_wrapper<IRenderableObject>>;

    // All objects, grouped by material
    using ObjectsByMaterial = std::map<OpenGLShader*, ObjectList>;

    // object mappings, grouped by entity
    std::map<IRenderEntity*, ObjectsByMaterial> _objectsByEntity;

    std::size_t _interactionDrawCalls;
    std::size_t _depthDrawCalls;
    std::size_t _objectCount;

public:
    LightInteractions(RendererLight& light, IGeometryStore& store) :
        _light(light),
        _store(store),
        _lightBounds(light.lightAABB()),
        _interactionDrawCalls(0),
        _depthDrawCalls(0),
        _objectCount(0)
    {}

    std::size_t getInteractionDrawCalls() const
    {
        return _interactionDrawCalls;
    }

    std::size_t getDepthDrawCalls() const
    {
        return _depthDrawCalls;
    }

    std::size_t getObjectCount() const
    {
        return _objectCount;
    }

    std::size_t getEntityCount() const
    {
        return _objectsByEntity.size();
    }

    void addObject(IRenderableObject& object, IRenderEntity& entity, OpenGLShader* shader);

    bool isInView(const IRenderView& view);

    void collectSurfaces(const IRenderView& view, const std::set<IRenderEntityPtr>& entities);

    void fillDepthBuffer(OpenGLState& state, GLSLDepthFillAlphaProgram& program, 
        const IRenderView& view, std::size_t renderTime, std::vector<IGeometryStore::Slot>& untransformedObjectsWithoutAlphaTest);

    void drawInteractions(OpenGLState& state, GLSLBumpProgram& program, const IRenderView& view, std::size_t renderTime);
};

}
