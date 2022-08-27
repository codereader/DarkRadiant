#pragma once

#include <map>
#include <vector>
#include <set>
#include "irender.h"
#include "irenderableobject.h"
#include "iobjectrenderer.h"
#include "irenderview.h"
#include "render/Rectangle.h"
#include "InteractionPass.h"

namespace render
{

class OpenGLState;
class OpenGLShader;
class DepthFillAlphaProgram;
class InteractionProgram;
class ShadowMapProgram;
class DepthFillPass;

/**
 * Depth-buffer filling light with diffuse/bump/specular interactions
 * between this light and one or more entity renderables.
 * Objects are grouped by entity, then by shader.
 *
 * Instances only live through the course of a single render pass, therefore direct
 * references without ref-counting are used.
 */
class RegularLight
{
public:
    // A flat list of renderables
    using ObjectList = std::vector<std::reference_wrapper<IRenderableObject>>;

private:
    RendererLight& _light;
    IGeometryStore& _store;
    IObjectRenderer& _objectRenderer;
    AABB _lightBounds;

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

    // Helper object submitting a DBS interaction draw call
    class InteractionDrawCall
    {
    private:
        OpenGLState& _state;
        InteractionProgram& _program;

        IObjectRenderer& _objectRenderer;
        const Vector3& _worldLightOrigin;
        const Vector3& _viewer;

        const InteractionPass::Stage* _bump;
        const InteractionPass::Stage* _diffuse;
        const InteractionPass::Stage* _specular;

        std::vector<IGeometryStore::Slot> _untransformedObjects;

        InteractionPass::Stage _defaultBumpStage;
        InteractionPass::Stage _defaultDiffuseStage;
        InteractionPass::Stage _defaultSpecularStage;

        std::size_t _interactionDrawCalls;

    public:
        InteractionDrawCall(OpenGLState& state, InteractionProgram& program,
            IObjectRenderer& objectRenderer, const Vector3& worldLightOrigin, const Vector3& viewer);

        std::size_t getInteractionDrawCalls() const
        {
            return _interactionDrawCalls;
        }

        void prepare(InteractionPass& pass)
        {
            _bump = nullptr;
            _diffuse = nullptr;
            _specular = nullptr;

            _defaultBumpStage.texture = pass.getDefaultInteractionTextureBinding(IShaderLayer::BUMP);
            _defaultDiffuseStage.texture = pass.getDefaultInteractionTextureBinding(IShaderLayer::DIFFUSE);
            _defaultSpecularStage.texture = pass.getDefaultInteractionTextureBinding(IShaderLayer::SPECULAR);
        }

        bool hasBump() const
        {
            return _bump != nullptr;
        }

        bool hasDiffuse() const
        {
            return _diffuse != nullptr;
        }

        bool hasSpecular() const
        {
            return _specular != nullptr;
        }

        void setBump(const InteractionPass::Stage* bump);
        void setDiffuse(const InteractionPass::Stage* diffuse);
        void setSpecular(const InteractionPass::Stage* specular);

        void submit(const ObjectList& objects);
    };

public:
    RegularLight(RendererLight& light, IGeometryStore& store, IObjectRenderer& objectRenderer);
    RegularLight(RegularLight&& other) = default;

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
