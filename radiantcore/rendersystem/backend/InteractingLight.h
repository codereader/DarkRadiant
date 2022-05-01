#pragma once

#include <map>
#include <vector>
#include <set>
#include "irender.h"
#include "irenderableobject.h"
#include "iobjectrenderer.h"
#include "irenderview.h"
#include "render/Rectangle.h"

namespace render
{

class OpenGLState;
class OpenGLShader;
class DepthFillAlphaProgram;
class InteractionProgram;
class ShadowMapProgram;
class DepthFillPass;

/**
 * Defines interactions between a light and one or more entity renderables
 * It only lives through the course of a single render pass, therefore direct
 * references without ref-counting are used.
 * 
 * Objects are grouped by entity, then by shader.
 */
class InteractingLight
{
private:
    RendererLight& _light;
    IGeometryStore& _store;
    IObjectRenderer& _objectRenderer;
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
    std::size_t _shadowMapDrawCalls;

    int _shadowLightIndex;
    bool _isShadowCasting;

public:
    InteractingLight(RendererLight& light, IGeometryStore& store, IObjectRenderer& objectRenderer);

    const Vector3& getBoundsCenter() const
    {
        return _lightBounds.getOrigin();
    }

    int getShadowLightIndex() const
    {
        return _shadowLightIndex;
    }

    void setShadowLightIndex(int index)
    {
        _shadowLightIndex = index;
    }

    const RendererLight& getLight() const
    {
        return _light;
    }

    std::size_t getInteractionDrawCalls() const
    {
        return _interactionDrawCalls;
    }

    std::size_t getDepthDrawCalls() const
    {
        return _depthDrawCalls;
    }

    std::size_t getShadowMapDrawCalls() const
    {
        return _shadowMapDrawCalls;
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

    bool isShadowCasting() const;

    void collectSurfaces(const IRenderView& view, const std::set<IRenderEntityPtr>& entities);

    void fillDepthBuffer(OpenGLState& state, DepthFillAlphaProgram& program, 
        std::size_t renderTime, std::vector<IGeometryStore::Slot>& untransformedObjectsWithoutAlphaTest);

    void drawShadowMap(OpenGLState& state, const Rectangle& rectangle, ShadowMapProgram& program, std::size_t renderTime);

    void drawInteractions(OpenGLState& state, InteractionProgram& program, const IRenderView& view, std::size_t renderTime);

    void setupAlphaTest(OpenGLState& state, OpenGLShader* shader, DepthFillPass* depthFillPass,
        ISupportsAlphaTest& alphaTestProgram, std::size_t renderTime, IRenderEntity* entity);
};

}
