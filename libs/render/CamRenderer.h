#pragma once

#include "irenderable.h"
#include "ivolumetest.h"

#include "VectorLightList.h"

#include <map>

namespace render
{

/// RenderableCollector for use with 3D camera views or preview widgets
class CamRenderer: public RenderableCollector
{
    // The VolumeTest object for object culling
    const VolumeTest& _view;

    // Render statistics
    int _totalLights = 0;
    int _visibleLights = 0;

    // Highlight state
    bool _highlightFaces = false;
    bool _highlightPrimitives = false;
    Shader* _highlightedPrimitiveShader = nullptr;
    Shader* _highlightedFaceShader = nullptr;

    // All lights we have received from the scene
    std::list<const RendererLight*> _sceneLights;

    // Legacy lit renderable which provided its own LightSources to
    // addRenderable()
    struct LegacyLitRenderable
    {
        const OpenGLRenderable& renderable;
        const LightSources* lights = nullptr;
        Matrix4 local2World;
        const IRenderEntity* entity = nullptr;

        LegacyLitRenderable(const OpenGLRenderable& r, const LightSources* l,
                            const Matrix4& l2w, const IRenderEntity* e)
        : renderable(r), lights(l), local2World(l2w), entity(e)
        {}
    };
    using LegacyLitRenderables = std::vector<LegacyLitRenderable>;

    // Legacy renderables with their own light lists. No processing needed;
    // just store them until it's time to submit to shaders.
    using LegacyRenderablesByShader = std::map<Shader*, LegacyLitRenderables>;
    LegacyRenderablesByShader _legacyRenderables;

    // Lit renderable provided via addLitRenderable(), for which we construct
    // the light list with lights received via addLight().
    struct LitRenderable
    {
        // Renderable information submitted with addLitObject()
        const OpenGLRenderable& renderable;
        const LitObject& litObject;
        Matrix4 local2World;
        const IRenderEntity* entity = nullptr;

        // Calculated list of intersecting lights (initially empty)
        render::lib::VectorLightList lights;
    };
    using LitRenderables = std::vector<LitRenderable>;

    // Renderables added with addLitObject() need to be stored until their
    // light lists can be calculated, which can't happen until all the lights
    // are submitted too.
    std::map<Shader*, LitRenderables> _litRenderables;

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
                    if (j->litObject.intersectsLight(*l))
                        j->lights.addLight(*l);
                }
            }
        }
    }

public:

    // Initialise CamRenderer with optional highlight shaders
    CamRenderer(const VolumeTest& view, Shader* primHighlightShader = nullptr,
                Shader* faceHighlightShader = nullptr)
    : _view(view),
      _highlightedPrimitiveShader(primHighlightShader),
      _highlightedFaceShader(faceHighlightShader)
    {}

    // Instruct the CamRenderer to push its sorted renderables to their
    // respective shaders
    void submitToShaders()
    {
        // Calculate intersections between lights and renderables we have received
        calculateLightIntersections();

        // Render legacy renderables with submitted light lists
        for (auto i = _legacyRenderables.begin();
             i != _legacyRenderables.end();
             ++i)
        {
            // Iterate over the list of renderables for this shader, submitting
            // each one
            Shader* shader = i->first;
            wxASSERT(shader);
            for (auto j = i->second.begin(); j != i->second.end(); ++j)
            {
                const LegacyLitRenderable& lr = *j;
                shader->addRenderable(lr.renderable, lr.local2World,
                                      lr.lights, lr.entity);
            }
        }

        // Render objects with calculated light lists
        for (auto i = _litRenderables.begin(); i != _litRenderables.end(); ++i)
        {
            Shader* shader = i->first;
            wxASSERT(shader);
            for (auto j = i->second.begin(); j != i->second.end(); ++j)
            {
                const LitRenderable& lr = *j;
                shader->addRenderable(lr.renderable, lr.local2World,
                                      &lr.lights, lr.entity);
            }
        }
    }

    /// Obtain the visible light count
    int getVisibleLights() const { return _visibleLights; }

    /// Obtain the total light count
    int getTotalLights() const { return _totalLights; }

    // RenderableCollector implementation

    bool supportsFullMaterials() const override { return true; }

    void setHighlightFlag(Highlight::Flags flags, bool enabled) override
    {
        if (flags & Highlight::Faces)
        {
            _highlightFaces = enabled;
        }

        if (flags & Highlight::Primitives)
        {
            _highlightPrimitives = enabled;
        }
    }

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

    void addRenderable(Shader& shader, const OpenGLRenderable& renderable,
                       const Matrix4& world, const LightSources* lights,
                       const IRenderEntity* entity) override
    {
        if (_highlightPrimitives && _highlightedPrimitiveShader)
            _highlightedPrimitiveShader->addRenderable(renderable, world,
                                                       lights, entity);

        if (_highlightFaces && _highlightedFaceShader)
            _highlightedFaceShader->addRenderable(renderable, world,
                                                 lights, entity);

        // Construct an entry for this shader in the map if it is the first
        // time we've seen it
        auto iter = _legacyRenderables.find(&shader);
        if (iter == _legacyRenderables.end())
        {
            // Add an entry for this shader, and pre-allocate some space in the
            // vector to avoid too many expansions during scenegraph traversal.
            LegacyLitRenderables emptyList;
            emptyList.reserve(1024);

            auto result = _legacyRenderables.insert(
                std::make_pair(&shader, std::move(emptyList))
            );
            wxASSERT(result.second);
            iter = result.first;
        }
        wxASSERT(iter != _legacyRenderables.end());

        // Add the renderable and its lights to the list of lit renderables for
        // this shader
        wxASSERT(iter->first == &shader);
        iter->second.emplace_back(renderable, lights, world, entity);
    }

    void addLitRenderable(Shader& shader,
                          const OpenGLRenderable& renderable,
                          const Matrix4& localToWorld,
                          const LitObject& litObject,
                          const IRenderEntity* entity = nullptr) override
    {
        if (_highlightPrimitives && _highlightedPrimitiveShader)
            _highlightedPrimitiveShader->addRenderable(renderable, localToWorld,
                                                       nullptr, entity);

        if (_highlightFaces && _highlightedFaceShader)
            _highlightedFaceShader->addRenderable(renderable, localToWorld,
                                                  nullptr, entity);

        // Construct an entry for this shader in the map if it is the first
        // time we've seen it
        auto iter = _litRenderables.find(&shader);
        if (iter == _litRenderables.end())
        {
            // Add an entry for this shader, and pre-allocate some space in the
            // vector to avoid too many expansions during scenegraph traversal.
            LitRenderables emptyList;
            emptyList.reserve(1024);

            auto result = _litRenderables.insert(
                std::make_pair(&shader, std::move(emptyList))
            );
            wxASSERT(result.second);
            iter = result.first;
        }
        wxASSERT(iter != _litRenderables.end());
        wxASSERT(iter->first == &shader);

        // Store a LitRenderable object for this renderable
        LitRenderable lr { renderable, litObject, localToWorld, entity };
        iter->second.push_back(std::move(lr));
    }
};


}
