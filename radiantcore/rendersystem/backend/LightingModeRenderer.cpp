#include "LightingModeRenderer.h"

#include "GLProgramFactory.h"
#include "LightingModeRenderResult.h"
#include "InteractingLight.h"
#include "OpenGLShaderPass.h"
#include "OpenGLShader.h"
#include "ObjectRenderer.h"
#include "OpenGLState.h"
#include "glprogram/CubeMapProgram.h"
#include "glprogram/DepthFillAlphaProgram.h"
#include "glprogram/InteractionProgram.h"
#include "glprogram/RegularStageProgram.h"

namespace render
{

LightingModeRenderer::LightingModeRenderer(GLProgramFactory& programFactory,
        IGeometryStore& store, IObjectRenderer& objectRenderer, 
        const std::set<RendererLightPtr>& lights,
        const std::set<IRenderEntityPtr>& entities) :
    SceneRenderer(RenderViewType::Camera),
    _programFactory(programFactory),
    _geometryStore(store),
    _objectRenderer(objectRenderer),
    _lights(lights),
    _entities(entities),
    _shadowMapProgram(nullptr),
    _shadowMappingEnabled(RKEY_ENABLE_SHADOW_MAPPING)
{
    _untransformedObjectsWithoutAlphaTest.reserve(10000);
    _nearestShadowLights.reserve(MaxShadowCastingLights + 1);
}

void LightingModeRenderer::ensureShadowMapSetup()
{
    if (!_shadowMappingEnabled.get()) return;

    if (!_shadowMapFbo)
    {
        _shadowMapFbo = FrameBuffer::CreateShadowMapBuffer();

        // Define the shadow atlas regions (supporting 6 lights)
        _shadowMapAtlas.resize(6);

        for (int i = 0; i < 6; ++i)
        {
            _shadowMapAtlas[i].x = 0;
            _shadowMapAtlas[i].y = static_cast<int>((_shadowMapFbo->getHeight() / 6) * i);
            _shadowMapAtlas[i].width = static_cast<int>(_shadowMapFbo->getWidth() / 6);
            _shadowMapAtlas[i].height = static_cast<int>(_shadowMapFbo->getHeight() / 6);
        }
    }

    if (!_shadowMapProgram)
    {
        _shadowMapProgram = dynamic_cast<ShadowMapProgram*>(_programFactory.getBuiltInProgram(ShaderProgram::ShadowMap));
        assert(_shadowMapProgram);
    }
}

IRenderResult::Ptr LightingModeRenderer::render(RenderStateFlags globalFlagsMask, 
    const IRenderView& view, std::size_t time)
{
    _result = std::make_shared<LightingModeRenderResult>();

    ensureShadowMapSetup();

    determineInteractingLight(view);

    // Construct default OpenGL state
    OpenGLState current;
    setupState(current);

    // Past this point, everything in the geometry store is up to date
    _geometryStore.syncToBufferObjects();

    auto [vertexBuffer, indexBuffer] = _geometryStore.getBufferObjects();

    vertexBuffer->bind();
    indexBuffer->bind();

    // Set the vertex attribute pointers
    _objectRenderer.initAttributePointers();

    // Render depth information to the shadow maps
    drawShadowMaps(current, time);

    // Load the model view & projection matrix for the main scene
    setupViewMatrices(view);

    // Run the depth fill pass
    drawDepthFillPass(current, globalFlagsMask, view, time);

    // Draw the surfaces per light and material
    drawInteractingLights(current, globalFlagsMask, view, time);

    // Draw any surfaces without any light interactions
    drawNonInteractionPasses(current, globalFlagsMask, view, time);

    vertexBuffer->unbind();
    indexBuffer->unbind();

    cleanupState();

    // Cleanup the data accumulated in this render pass
    _interactingLights.clear();
    _nearestShadowLights.clear();

    return std::move(_result); // move-return our result reference
}

void LightingModeRenderer::determineInteractingLight(const IRenderView& view)
{
    _interactingLights.reserve(_lights.size());

    // Gather all visible lights and render the surfaces touched by them
    for (const auto& light : _lights)
    {
        InteractingLight interaction(*light, _geometryStore, _objectRenderer);

        if (!interaction.isInView(view))
        {
            _result->skippedLights++;
            continue;
        }

        _result->visibleLights++;

        // Check all the surfaces that are touching this light
        interaction.collectSurfaces(view, _entities);

        _result->objects += interaction.getObjectCount();
        _result->entities += interaction.getEntityCount();

        // Move the interaction list into its place
        auto& moved = _interactingLights.emplace_back(std::move(interaction));

        // Check the distance of shadow casting lights to the viewer
        if (_shadowMappingEnabled.get() && moved.isShadowCasting())
        {
            addToShadowLights(moved, view.getViewer());
        }
    }

    // Assign shadow light indices
    for (auto index = 0; index < _nearestShadowLights.size(); ++index)
    {
        _nearestShadowLights[index]->setShadowLightIndex(index);
    }
}

void LightingModeRenderer::addToShadowLights(InteractingLight& light, const Vector3& viewer)
{
    if (_nearestShadowLights.empty())
    {
        _nearestShadowLights.push_back(&light);
        return;
    }

    auto distance = (light.getBoundsCenter() - viewer).getLengthSquared();

    for (auto other = _nearestShadowLights.begin(); other != _nearestShadowLights.end(); ++other)
    {
        if (((*other)->getBoundsCenter() - viewer).getLengthSquared() > distance)
        {
            // Insert here
            _nearestShadowLights.insert(other, &light);

            if (_nearestShadowLights.size() > MaxShadowCastingLights)
            {
                _nearestShadowLights.pop_back();
            }
            return;
        }
    }

    // All existing lights are nearer, is there room at the end?
    if (_nearestShadowLights.size() < MaxShadowCastingLights)
    {
        _nearestShadowLights.push_back(&light);
    }
}

void LightingModeRenderer::drawInteractingLights(OpenGLState& current, RenderStateFlags globalFlagsMask,
    const IRenderView& view, std::size_t renderTime)
{
    // Draw the surfaces per light and material
    auto interactionState = InteractionPass::GenerateInteractionState(_programFactory);

    // Prepare the current state for drawing
    interactionState.applyTo(current, globalFlagsMask);

    auto interactionProgram = dynamic_cast<InteractionProgram*>(current.glProgram);
    assert(interactionProgram);

    interactionProgram->setModelViewProjection(view.GetViewProjection());

    if (_shadowMappingEnabled.get())
    {
        // Bind the texture containing the shadow maps
        OpenGLState::SetTextureState(current.texture5, _shadowMapFbo->getTextureNumber(), GL_TEXTURE5, GL_TEXTURE_2D);
    }

    for (auto& interactionList : _interactingLights)
    {
        auto shadowLightIndex = interactionList.getShadowLightIndex();

        if (shadowLightIndex != -1)
        {
            // Define which part of the shadow map atlas should be sampled
            interactionProgram->enableShadowMapping(true);
            interactionProgram->setShadowMapRectangle(_shadowMapAtlas[shadowLightIndex]);
        }
        else
        {
            interactionProgram->enableShadowMapping(false);
        }

        interactionList.drawInteractions(current, *interactionProgram, view, renderTime);
        _result->interactionDrawCalls += interactionList.getInteractionDrawCalls();
    }

    if (_shadowMappingEnabled.get())
    {
        // Unbind the shadow map texture
        OpenGLState::SetTextureState(current.texture5, 0, GL_TEXTURE5, GL_TEXTURE_2D);
    }
}

void LightingModeRenderer::drawShadowMaps(OpenGLState& current,std::size_t renderTime)
{
    if (!_shadowMappingEnabled.get()) return;

    // Draw the shadow maps of each light
    // Save the viewport set up in the camera code
    GLint previousViewport[4];
    glGetIntegerv(GL_VIEWPORT, previousViewport);

    _shadowMapProgram->enable();
    _shadowMapFbo->bind();

    // Enable GL state and save to state
    glDepthMask(GL_TRUE);
    current.setRenderFlag(RENDER_DEPTHWRITE);

    glDepthFunc(GL_LEQUAL);
    current.setDepthFunc(GL_LEQUAL);

    glEnable(GL_DEPTH_TEST);
    current.setRenderFlag(RENDER_DEPTHTEST);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    current.setRenderFlag(RENDER_FILL);

    glPolygonOffset(0, 0);
    glEnable(GL_POLYGON_OFFSET_FILL);

    // Enable the 4 clip planes, they are used in the vertex shader
    glEnable(GL_CLIP_DISTANCE0);
    glEnable(GL_CLIP_DISTANCE1);
    glEnable(GL_CLIP_DISTANCE2);
    glEnable(GL_CLIP_DISTANCE3);

    glViewport(0, 0, static_cast<GLsizei>(_shadowMapFbo->getWidth()), static_cast<GLsizei>(_shadowMapFbo->getHeight()));
    glClear(GL_DEPTH_BUFFER_BIT);

    // Render shadow casting lights to the shadow map buffer, up to MaxShadowLightCount
    for (auto light : _nearestShadowLights)
    {
        light->drawShadowMap(current, _shadowMapAtlas[light->getShadowLightIndex()], *_shadowMapProgram, renderTime);
        _result->shadowDrawCalls += light->getShadowMapDrawCalls();
    }

    _shadowMapFbo->unbind();
    _shadowMapProgram->disable();

    glDisable(GL_CLIP_DISTANCE3);
    glDisable(GL_CLIP_DISTANCE2);
    glDisable(GL_CLIP_DISTANCE1);
    glDisable(GL_CLIP_DISTANCE0);

    glDisable(GL_POLYGON_OFFSET_FILL);

    // Restore view port
    glViewport(previousViewport[0], previousViewport[1], previousViewport[2], previousViewport[3]);

    glDisable(GL_DEPTH_TEST);
    current.clearRenderFlag(RENDER_DEPTHTEST);
}

void LightingModeRenderer::drawDepthFillPass(OpenGLState& current, RenderStateFlags globalFlagsMask,
    const IRenderView& view, std::size_t renderTime)
{
    // Run the depth fill pass
    auto depthFillState = DepthFillPass::GenerateDepthFillState(_programFactory);

    // Prepare the current state for depth filling
    depthFillState.applyTo(current, globalFlagsMask);

    auto depthFillProgram = dynamic_cast<DepthFillAlphaProgram*>(current.glProgram);
    assert(depthFillProgram);

    // Set the modelview and projection matrix
    depthFillProgram->setModelViewProjection(view.GetViewProjection());

    for (auto& interactionList : _interactingLights)
    {
        interactionList.fillDepthBuffer(current, *depthFillProgram, renderTime, _untransformedObjectsWithoutAlphaTest);
        _result->depthDrawCalls += interactionList.getDepthDrawCalls();
    }

    // Unbind the diffuse texture
    OpenGLState::SetTextureState(current.texture0, 0, GL_TEXTURE0, GL_TEXTURE_2D);

    // All objects without alpha test or transformation matrix go into one final drawcall
    if (!_untransformedObjectsWithoutAlphaTest.empty())
    {
        depthFillProgram->setObjectTransform(Matrix4::getIdentity());
        depthFillProgram->setAlphaTest(-1);

        _objectRenderer.submitGeometry(_untransformedObjectsWithoutAlphaTest, GL_TRIANGLES);
        _result->depthDrawCalls++;

        _untransformedObjectsWithoutAlphaTest.clear();
    }
}

void LightingModeRenderer::drawNonInteractionPasses(OpenGLState& current, RenderStateFlags globalFlagsMask, 
    const IRenderView& view, std::size_t time)
{
    glUseProgram(0);
    glActiveTexture(GL_TEXTURE0);
    glClientActiveTexture(GL_TEXTURE0);

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);

    // Draw non-interaction passes (like skyboxes or blend stages)
    for (const auto& entity : _entities)
    {
        entity->foreachRenderable([&](const IRenderableObject::Ptr& object, Shader* shader)
        {
            // Skip empty objects
            if (!object->isVisible()) return;

            // Don't collect invisible shaders
            if (!shader->isVisible()) return;

            auto glShader = static_cast<OpenGLShader*>(shader);

            // We only consider materials designated for camera rendering
            if (!glShader->isApplicableTo(RenderViewType::Camera))
            {
                return;
            }

            // For each pass except for the depth fill and interaction passes, draw the geometry
            glShader->foreachNonInteractionPass([&](OpenGLShaderPass& pass)
            {
                if (!pass.stateIsActive())
                {
                    return;
                }

                // Apply our state to the current state object
                pass.evaluateStagesAndApplyState(current, globalFlagsMask, time, entity.get());

                // Bind textures
                OpenGLState::SetTextureState(current.texture0, pass.state().texture0, GL_TEXTURE0, GL_TEXTURE_2D);

                if (dynamic_cast<RegularStageProgram*>(current.glProgram))
                {
                    auto program = static_cast<RegularStageProgram*>(current.glProgram);

                    program->setModelViewProjection(view.GetViewProjection());
                    program->setObjectTransform(object->getObjectTransform());
                    program->setStageVertexColour(pass.state().getVertexColourMode(), pass.state().getColour());

                    const auto& diffuse = pass.state().stage0;
                    program->setDiffuseTextureTransform(diffuse ? diffuse->getTextureTransform() : Matrix4::getIdentity());
                }
                else if (dynamic_cast<CubeMapProgram*>(current.glProgram))
                {
                    static_cast<CubeMapProgram*>(current.glProgram)->setViewer(view.getViewer());
                }

                _objectRenderer.submitGeometry(object->getStorageLocation(), GL_TRIANGLES);
                _result->nonInteractionDrawCalls++;
            });
        });
    }

    OpenGLState::SetTextureState(current.texture0, 0, GL_TEXTURE0, GL_TEXTURE_2D);
}

}
