#include "LightingModeRenderer.h"

#include "LightingModeRenderResult.h"
#include "LightInteractions.h"
#include "OpenGLShaderPass.h"
#include "OpenGLShader.h"
#include "ObjectRenderer.h"
#include "OpenGLState.h"
#include "glprogram/GLSLDepthFillAlphaProgram.h"
#include "glprogram/GLSLBumpProgram.h"

namespace render
{

IRenderResult::Ptr LightingModeRenderer::render(RenderStateFlags globalFlagsMask, 
    const IRenderView& view, std::size_t time)
{
    auto result = std::make_shared<LightingModeRenderResult>();

    // Construct default OpenGL state
    OpenGLState current;
    setupState(current);
    setupViewMatrices(view);

    std::vector<LightInteractions> interactionLists;
    interactionLists.reserve(_lights.size());

    // Gather all visible lights and render the surfaces touched by them
    for (const auto& light : _lights)
    {
        LightInteractions interaction(*light, _geometryStore);

        if (!interaction.isInView(view))
        {
            result->skippedLights++;
            continue;
        }

        result->visibleLights++;

        // Check all the surfaces that are touching this light
        interaction.collectSurfaces(view, _entities);

        result->objects += interaction.getObjectCount();
        result->entities += interaction.getEntityCount();

        interactionLists.emplace_back(std::move(interaction));
    }

    // Past this point, everything in the geometry store is up to date
    _geometryStore.syncToBufferObjects();

    auto [vertexBuffer, indexBuffer] = _geometryStore.getBufferObjects();

    vertexBuffer->bind();
    indexBuffer->bind();

    // Set the vertex attribute pointers
    ObjectRenderer::InitAttributePointers();

    result->depthDrawCalls += drawDepthFillPass(current, globalFlagsMask, interactionLists, view, time);

    // Draw the surfaces per light and material
    auto interactionState = InteractionPass::GenerateInteractionState(_programFactory);

    // Prepare the current state for drawing
    interactionState.applyTo(current, globalFlagsMask);

    auto interactionProgram = dynamic_cast<GLSLBumpProgram*>(current.glProgram);
    assert(interactionProgram);

    interactionProgram->setModelViewProjection(view.GetViewProjection());

    for (auto& interactionList : interactionLists)
    {
        interactionList.drawInteractions(current, *interactionProgram, view, time);
        result->interactionDrawCalls += interactionList.getInteractionDrawCalls();
    }

    result->nonInteractionDrawCalls += drawNonInteractionPasses(current, globalFlagsMask, view, time);

    cleanupState();

    return result;
}

std::size_t LightingModeRenderer::drawDepthFillPass(OpenGLState& current, RenderStateFlags globalFlagsMask,
    std::vector<LightInteractions>& interactionLists, const IRenderView& view, std::size_t renderTime)
{
    std::size_t drawCalls = 0;

    // Run the depth fill pass
    auto depthFillState = DepthFillPass::GenerateDepthFillState(_programFactory);

    // Prepare the current state for depth filling
    depthFillState.applyTo(current, globalFlagsMask);

    auto depthFillProgram = dynamic_cast<GLSLDepthFillAlphaProgram*>(current.glProgram);
    assert(depthFillProgram);

    // Set the modelview and projection matrix
    depthFillProgram->setModelViewProjection(view.GetViewProjection());

    for (auto& interactionList : interactionLists)
    {
        interactionList.fillDepthBuffer(current, *depthFillProgram, view, renderTime, _untransformedObjectsWithoutAlphaTest);
        drawCalls += interactionList.getDepthDrawCalls();
    }

    // Unbind the diffuse texture
    OpenGLState::SetTextureState(current.texture0, 0, GL_TEXTURE0, GL_TEXTURE_2D);

    // All objects without alpha test or transformation matrix go into one final drawcall
    if (!_untransformedObjectsWithoutAlphaTest.empty())
    {
        depthFillProgram->setObjectTransform(Matrix4::getIdentity());
        depthFillProgram->setAlphaTest(-1);

        ObjectRenderer::SubmitGeometry(_untransformedObjectsWithoutAlphaTest, GL_TRIANGLES, _geometryStore);
        drawCalls++;

        _untransformedObjectsWithoutAlphaTest.clear();
    }

    return drawCalls;
}

std::size_t LightingModeRenderer::drawNonInteractionPasses(OpenGLState& current, RenderStateFlags globalFlagsMask, 
    const IRenderView& view, std::size_t time)
{
    std::size_t drawCalls = 0;

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
        entity->foreachRenderable([&](const render::IRenderableObject::Ptr& object, Shader* shader)
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

                if (current.glProgram)
                {
                    OpenGLShaderPass::SetUpNonInteractionProgram(current, view.getViewer(), object->getObjectTransform());
                }

                ObjectRenderer::SubmitObject(*object, _geometryStore);
                drawCalls++;
            });
        });
    }

    return drawCalls;
}

}