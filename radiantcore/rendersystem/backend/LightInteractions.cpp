#include "LightInteractions.h"

#include "OpenGLShader.h"
#include "ObjectRenderer.h"

namespace render
{

void LightInteractions::addObject(IRenderableObject& object, IRenderEntity& entity, OpenGLShader* shader)
{
    auto& objectsByMaterial = _objectsByEntity.emplace(
        &entity, ObjectsByMaterial{}).first->second;

    auto& surfaces = objectsByMaterial.emplace(
        shader, ObjectList{}).first->second;

    surfaces.emplace_back(std::ref(object));

    ++_objectCount;
}

bool LightInteractions::isInView(const IRenderView& view)
{
    return view.TestAABB(_lightBounds) != VOLUME_OUTSIDE;
}

void LightInteractions::collectSurfaces(const std::set<IRenderEntityPtr>& entities)
{
    // Now check all the entities intersecting with this light
    for (const auto& entity : entities)
    {
        entity->foreachRenderableTouchingBounds(_lightBounds,
            [&](const IRenderableObject::Ptr& object, Shader* shader)
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

            if (!glShader->getInteractionPass())
            {
                return; // This material doesn't interact with lighting
            }

            addObject(*object, *entity, glShader);
        });
    }
}

void LightInteractions::fillDepthBuffer(OpenGLState& state, RenderStateFlags globalFlagsMask, 
    const IRenderView& view, std::size_t renderTime)
{
    std::vector<IGeometryStore::Slot> untransformedObjects;
    untransformedObjects.reserve(10000);

    for (const auto& [entity, objectsByShader] : _objectsByEntity)
    {
        for (const auto& [shader, objects] : objectsByShader)
        {
            auto depthFillPass = shader->getDepthFillPass();

            if (!depthFillPass) continue;

            // Skip translucent materials
            if (shader->getMaterial() && shader->getMaterial()->getCoverage() == Material::MC_TRANSLUCENT)
            {
                continue;
            }

            // Evaluate the shader stages of this material
            depthFillPass->evaluateShaderStages(renderTime, entity);

            // Apply our state to the current state object
            depthFillPass->applyState(state, globalFlagsMask);

            auto depthFillProgram = depthFillPass->getDepthFillProgram();

            // Apply the evaluated alpha test value
            depthFillProgram.setAlphaTest(state.alphaThreshold);

            // Set the modelview and projection matrix
            depthFillProgram.setModelViewProjection(view.GetViewProjection());

            // Set the stage texture transformation matrix to the GLSL uniform
            // Since the texture matrix just needs 6 active components, we use two vec3
            if (depthFillPass->state().stage0)
            {
                auto textureMatrix = depthFillPass->state().stage0->getTextureTransform();
                depthFillProgram.setDiffuseTextureTransform(textureMatrix);
            }
            else
            {
                depthFillProgram.setDiffuseTextureTransform(Matrix4::getIdentity());
            }

            for (const auto& object : objects)
            {
                // We submit all objects with an identity matrix in a single multi draw call
                if (!object.get().isOriented())
                {
                    untransformedObjects.push_back(object.get().getStorageLocation());
                    continue;
                }

                depthFillProgram.setObjectTransform(object.get().getObjectTransform());

                ObjectRenderer::SubmitGeometry(object.get().getStorageLocation(), GL_TRIANGLES, _store);
                ++_drawCalls;
            }

            if (!untransformedObjects.empty())
            {
                depthFillProgram.setObjectTransform(Matrix4::getIdentity());

                ObjectRenderer::SubmitGeometry(untransformedObjects, GL_TRIANGLES, _store);
                ++_drawCalls;

                untransformedObjects.clear();
            }
        }
    }
}

void LightInteractions::render(OpenGLState& state, RenderStateFlags globalFlagsMask, const IRenderView& view, std::size_t renderTime)
{
    auto worldToLight = _light.getLightTextureTransformation();

    std::vector<IGeometryStore::Slot> untransformedObjects;
    untransformedObjects.reserve(10000);

    for (const auto& [entity, objectsByShader] : _objectsByEntity)
    {
        for (const auto& [shader, objects] : objectsByShader)
        {
            const auto pass = shader->getInteractionPass();

            if (pass && pass->stateIsActive())
            {
                // Apply our state to the current state object
                pass->evaluateStagesAndApplyState(state, globalFlagsMask, renderTime, entity);

                for (const auto& object : objects)
                {
                    // We submit all objects with an identity matrix in a single multi draw call
                    if (!object.get().isOriented())
                    {
                        untransformedObjects.push_back(object.get().getStorageLocation());
                        continue;
                    }

                    OpenGLShaderPass::SetUpLightingCalculation(state, &_light, worldToLight,
                        view.getViewer(), object.get().getObjectTransform(), renderTime);

                    pass->getProgram().setObjectTransform(object.get().getObjectTransform());

                    ObjectRenderer::SubmitGeometry(object.get().getStorageLocation(), GL_TRIANGLES, _store);
                    ++_drawCalls;
                }

                if (!untransformedObjects.empty())
                {
                    OpenGLShaderPass::SetUpLightingCalculation(state, &_light, worldToLight,
                        view.getViewer(), Matrix4::getIdentity(), renderTime);

                    pass->getProgram().setObjectTransform(Matrix4::getIdentity());

                    ObjectRenderer::SubmitGeometry(untransformedObjects, GL_TRIANGLES, _store);
                    ++_drawCalls;

                    untransformedObjects.clear();
                }
            }
        }
    }
}

}
