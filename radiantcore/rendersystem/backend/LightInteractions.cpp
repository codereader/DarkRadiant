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
            [&](const render::IRenderableObject::Ptr& object, Shader* shader)
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
    glEnableClientState(GL_VERTEX_ARRAY);

    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);

    std::vector<IGeometryStore::Slot> untransformedObjects;
    untransformedObjects.reserve(10000);

    for (auto& pair : _objectsByEntity)
    {
        auto entity = pair.first;

        for (auto& pair : pair.second)
        {
            auto shader = pair.first;

            if (!shader->getDepthFillPass()) continue;

            // Skip translucent materials
            if (shader->getMaterial() && shader->getMaterial()->getCoverage() == Material::MC_TRANSLUCENT)
            {
                continue;
            }
            
            auto& objectList = pair.second;

            // Apply our state to the current state object
            shader->getDepthFillPass()->applyState(state, globalFlagsMask, view.getViewer(), renderTime, entity);

            for (auto object : objectList)
            {
                // We submit all objects with an identity matrix in a single multi draw call
                if (!object.get().isOriented())
                {
                    untransformedObjects.push_back(object.get().getStorageLocation());
                    continue;
                }

                ObjectRenderer::SubmitObject(object.get(), _store);
                ++_drawCalls;
            }

            if (!untransformedObjects.empty())
            {
                ObjectRenderer::SubmitGeometry(untransformedObjects, GL_TRIANGLES, _store);
                ++_drawCalls;

                untransformedObjects.clear();
            }
        }
    }

    glDisableClientState(GL_VERTEX_ARRAY);
}

void LightInteractions::render(OpenGLState& state, RenderStateFlags globalFlagsMask, const IRenderView& view, std::size_t renderTime)
{
    auto worldToLight = _light.getLightTextureTransformation();

    std::vector<IGeometryStore::Slot> untransformedObjects;
    untransformedObjects.reserve(10000);

    for (auto& pair : _objectsByEntity)
    {
        auto entity = pair.first;

        for (auto& pair : pair.second)
        {
            auto shader = pair.first;

            auto pass = shader->getInteractionPass();

            if (pass && pass->stateIsActive())
            {
                // Apply our state to the current state object
                pass->applyState(state, globalFlagsMask, view.getViewer(), renderTime, entity);

                for (auto object : pair.second)
                {
                    // We submit all objects with an identity matrix in a single multi draw call
                    if (!object.get().isOriented())
                    {
                        untransformedObjects.push_back(object.get().getStorageLocation());
                        continue;
                    }

                    OpenGLShaderPass::setUpLightingCalculation(state, &_light, worldToLight,
                        view.getViewer(), object.get().getObjectTransform(), renderTime, state.isColourInverted());

                    ObjectRenderer::SubmitObject(object.get(), _store);
                    ++_drawCalls;
                }

                if (!untransformedObjects.empty())
                {
                    OpenGLShaderPass::setUpLightingCalculation(state, &_light, worldToLight,
                        view.getViewer(), Matrix4::getIdentity(), renderTime, state.isColourInverted());

                    ObjectRenderer::SubmitGeometry(untransformedObjects, GL_TRIANGLES, _store);
                    ++_drawCalls;

                    untransformedObjects.clear();
                }
            }
        }
    }
}

}
