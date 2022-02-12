#pragma once

#include "render/RenderableCollectorBase.h"
#include "imap.h"
#include "ivolumetest.h"

#include "VectorLightList.h"

#include <unordered_map>

namespace render
{

/// RenderableCollector for use with 3D camera views or preview widgets
class CamRenderer : 
    public RenderableCollectorBase
{
public:
    struct HighlightShaders
    {
        ShaderPtr faceHighlightShader;
        ShaderPtr primitiveHighlightShader;
        ShaderPtr mergeActionShaderAdd;
        ShaderPtr mergeActionShaderChange;
        ShaderPtr mergeActionShaderRemove;
        ShaderPtr mergeActionShaderConflict;
    };

private:
    // The VolumeTest object for object culling
    const VolumeTest& _view;

    IMap::EditMode _editMode;

    // Render statistics
    int _totalLights = 0;
    int _visibleLights = 0;

    const HighlightShaders& _shaders;

    // All lights we have received from the scene
    std::list<const RendererLight*> _sceneLights;

    // Lit renderable provided via addRenderable(), for which we construct the
    // light list with lights received via addLight().
    struct LitRenderable
    {
        // Renderable information submitted with addLitObject()
        const OpenGLRenderable& renderable;
        const LitObject* litObject = nullptr;
        Matrix4 local2World;
        const IRenderEntity* entity = nullptr;

        // Calculated list of intersecting lights (initially empty)
        render::lib::VectorLightList lights;
    };
    using LitRenderables = std::vector<LitRenderable>;

    // Renderables added with addLitObject() need to be stored until their
    // light lists can be calculated, which can't happen until all the lights
    // are submitted too.
    std::unordered_map<Shader*, LitRenderables> _litRenderables;

    // Intersect all received renderables wiith lights
    void calculateLightIntersections()
    {
        // For each shader
        for (auto i = _litRenderables.begin(); i != _litRenderables.end(); ++i)
        {
            // For each renderable associated with this shader
            for (auto j = i->second.begin(); j != i->second.end(); ++j)
            {
                // Test intersection between the LitObject and each light in
                // the scene
                for (const RendererLight* l: _sceneLights)
                {
                    if (j->litObject && j->litObject->intersectsLight(*l))
                        j->lights.addLight(*l);
                }
            }
        }
    }

public:

    /// Initialise CamRenderer with optional highlight shaders
    CamRenderer(const VolumeTest& view, const HighlightShaders& shaders)
    : _view(view),
      _editMode(GlobalMapModule().getEditMode()),
      _shaders(shaders)
    {}

    void prepare()
    {
        _totalLights = 0;
        _visibleLights = 0;
        _editMode = GlobalMapModule().getEditMode();
    }

    void cleanup()
    {
        // Keep the shader map intact, but clear the renderables vectors,
        // so that we don't have to re-allocate the whole memory every frame
        // Purge the ones that have not been used in this render round
        for (auto i = _litRenderables.begin(); i != _litRenderables.end();)
        {
            if (i->second.empty())
            {
                // This shader has not been used at all in the last frame, free the memory
                _litRenderables.erase(i++);
                continue;
            }

            (i++)->second.clear();
        }

        _sceneLights.clear();
    }

    /**
     * \brief
     * Instruct the CamRenderer to push its sorted renderables to their
     * respective shaders.
     *
     * \param useLights
     * true if lighting calculations should be performed and light lists sent
     * to shaders; false if lights should be ignored (e.g. in fullbright render
     * mode).
     */
    void submitToShaders(bool useLights = true)
    {
        if (useLights)
        {
            // Calculate intersections between lights and renderables we have
            // received
            calculateLightIntersections();
        }

        // Render objects with calculated light lists
        for (const auto& pair : _litRenderables)
        {
            Shader* shader = pair.first;
            assert(shader);
            for (const LitRenderable& lr : pair.second)
            {
                shader->addRenderable(lr.renderable, lr.local2World,
                                      useLights ? &lr.lights : nullptr,
                                      lr.entity);
            }
        }
    }

    /// Obtain the visible light count
    int getVisibleLights() const { return _visibleLights; }

    /// Obtain the total light count
    int getTotalLights() const { return _totalLights; }

    // RenderableCollector implementation

    bool supportsFullMaterials() const override { return true; }

    void addLight(const RendererLight& light) override
    {
        // Determine if this light is visible within the view frustum
        VolumeIntersectionValue viv = _view.TestAABB(light.lightAABB());
        if (viv != VOLUME_OUTSIDE)
        {
            // Store the light in our list of scene lights
            _sceneLights.push_back(&light);

            // Count the light for the stats display
            ++_visibleLights;
        }

        // Count total lights
        ++_totalLights;
    }

    void addRenderable(Shader& shader,
                       const OpenGLRenderable& renderable,
                       const Matrix4& localToWorld,
                       const LitObject* litObject = nullptr,
                       const IRenderEntity* entity = nullptr) override
    {
        addHighlightRenderable(renderable, localToWorld);

        // Construct an entry for this shader in the map if it is the first
        // time we've seen it
        auto iter = _litRenderables.find(&shader);
        if (iter == _litRenderables.end())
        {
            // Add an entry for this shader, and pre-allocate some space in the
            // vector to avoid too many expansions during scenegraph traversal.
            LitRenderables emptyList;
            emptyList.reserve(1024);

            auto result = _litRenderables.emplace(&shader, std::move(emptyList));
            assert(result.second);
            iter = result.first;
        }
        assert(iter != _litRenderables.end());
        assert(iter->first == &shader);

        // Store a LitRenderable object for this renderable
        LitRenderable lr { renderable, litObject, localToWorld, entity };
        iter->second.emplace_back(std::move(lr));
    }

    void addHighlightRenderable(const OpenGLRenderable& renderable, const Matrix4& localToWorld) override
    {
        if (_editMode == IMap::EditMode::Merge && (_flags & Highlight::Flags::MergeAction) != 0)
        {
            const auto& mergeShader = (_flags & Highlight::Flags::MergeActionAdd) != 0 ? _shaders.mergeActionShaderAdd :
                (_flags & Highlight::Flags::MergeActionRemove) != 0 ? _shaders.mergeActionShaderRemove :
                (_flags & Highlight::Flags::MergeActionConflict) != 0 ? _shaders.mergeActionShaderConflict : _shaders.mergeActionShaderChange;

            if (mergeShader)
            {
                mergeShader->addRenderable(renderable, localToWorld, nullptr, nullptr);
            }
        }

        if ((_flags & Highlight::Flags::Primitives) != 0 && _shaders.primitiveHighlightShader)
        {
            _shaders.primitiveHighlightShader->addRenderable(renderable, localToWorld, nullptr, nullptr);
        }

        if ((_flags & Highlight::Flags::Faces) != 0 && _shaders.faceHighlightShader)
        {
            _shaders.faceHighlightShader->addRenderable(renderable, localToWorld, nullptr, nullptr);
        }
    }
};


}
