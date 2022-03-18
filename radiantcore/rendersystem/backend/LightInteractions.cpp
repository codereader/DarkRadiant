#include "LightInteractions.h"

#include "OpenGLShader.h"
#include "ObjectRenderer.h"
#include "glprogram/GLSLDepthFillAlphaProgram.h"

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

void LightInteractions::fillDepthBuffer(OpenGLState& state, GLSLDepthFillAlphaProgram& program, const IRenderView& view, std::size_t renderTime)
{
    std::vector<IGeometryStore::Slot> untransformedObjects;
    untransformedObjects.reserve(10000);

    // Set the modelview and projection matrix
    program.setModelViewProjection(view.GetViewProjection());

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

            // Apply the alpha test value, it might be affected by time and entity parms
            program.setAlphaTest(depthFillPass->getAlphaTestValue());

            // If there's a diffuse stage, apply the correct texture
            OpenGLState::SetTextureState(state.texture0, depthFillPass->state().texture0, GL_TEXTURE0, GL_TEXTURE_2D);

            // Set evaluated stage texture transformation matrix to the GLSL uniform
            program.setDiffuseTextureTransform(depthFillPass->getDiffuseTextureTransform());

            for (const auto& object : objects)
            {
                // We submit all objects with an identity matrix in a single multi draw call
                if (!object.get().isOriented())
                {
                    untransformedObjects.push_back(object.get().getStorageLocation());
                    continue;
                }

                program.setObjectTransform(object.get().getObjectTransform());

                ObjectRenderer::SubmitGeometry(object.get().getStorageLocation(), GL_TRIANGLES, _store);
                ++_drawCalls;
            }

            if (!untransformedObjects.empty())
            {
                program.setObjectTransform(Matrix4::getIdentity());

                ObjectRenderer::SubmitGeometry(untransformedObjects, GL_TRIANGLES, _store);
                ++_drawCalls;

                untransformedObjects.clear();
            }
        }
    }
}

void LightInteractions::drawInteractions(OpenGLState& state, GLSLBumpProgram& program, 
    const IRenderView& view, std::size_t renderTime)
{
    if (_objectsByEntity.empty())
    {
        return;
    }

    auto worldToLight = _light.getLightTextureTransformation();
    auto worldLightOrigin = _light.getLightOrigin();

    std::vector<IGeometryStore::Slot> untransformedObjects;
    untransformedObjects.reserve(10000);

    program.setModelViewProjection(view.GetViewProjection());

    // Set up textures used by this light
    program.setupLightParameters(state, _light, renderTime);

    for (const auto& [entity, objectsByShader] : _objectsByEntity)
    {
        for (const auto& [shader, objects] : objectsByShader)
        {
            const auto pass = shader->getInteractionPass();

            if (!pass || !pass->stateIsActive()) continue;

            // Evaluate the expressions in the material stages
            pass->evaluateShaderStages(renderTime, entity);

            // Enable alphatest if required
            if (pass->state().stage0 && pass->state().stage0->hasAlphaTest())
            {
                glEnable(GL_ALPHA_TEST);
                glAlphaFunc(GL_GEQUAL, pass->state().stage0->getAlphaTest());
            }
            else
            {
                glDisable(GL_ALPHA_TEST);
            }

            // Bind textures
            OpenGLState::SetTextureState(state.texture0, pass->state().texture0, GL_TEXTURE0, GL_TEXTURE_2D);
            OpenGLState::SetTextureState(state.texture1, pass->state().texture1, GL_TEXTURE1, GL_TEXTURE_2D);
            OpenGLState::SetTextureState(state.texture2, pass->state().texture2, GL_TEXTURE2, GL_TEXTURE_2D);

            // Load stage texture matrices
            program.setDiffuseTextureTransform(pass->getDiffuseTextureTransform());
            program.setBumpTextureTransform(pass->getBumpTextureTransform());
            program.setSpecularTextureTransform(pass->getSpecularTextureTransform());

            // Vertex colour mode and diffuse stage colour setup for this pass
            program.setStageVertexColour(pass->state().getVertexColourMode(), 
                pass->state().stage0 ? pass->state().stage0->getColour() : Colour4::WHITE());

            for (const auto& object : objects)
            {
                // We submit all objects with an identity matrix in a single multi draw call
                if (!object.get().isOriented())
                {
                    untransformedObjects.push_back(object.get().getStorageLocation());
                    continue;
                }

                program.setUpObjectLighting(worldLightOrigin, worldToLight,
                    view.getViewer(), object.get().getObjectTransform(), 
                    object.get().getObjectTransform().getInverse());

                pass->getProgram().setObjectTransform(object.get().getObjectTransform());

                ObjectRenderer::SubmitGeometry(object.get().getStorageLocation(), GL_TRIANGLES, _store);
                ++_drawCalls;
            }

            if (!untransformedObjects.empty())
            {
                program.setUpObjectLighting(worldLightOrigin, worldToLight,
                    view.getViewer(), Matrix4::getIdentity(), Matrix4::getIdentity());

                pass->getProgram().setObjectTransform(Matrix4::getIdentity());

                ObjectRenderer::SubmitGeometry(untransformedObjects, GL_TRIANGLES, _store);
                ++_drawCalls;

                untransformedObjects.clear();
            }
        }
    }

    // Unbind the light textures
    OpenGLState::SetTextureState(state.texture3, 0, GL_TEXTURE3, GL_TEXTURE_2D);
    OpenGLState::SetTextureState(state.texture4, 0, GL_TEXTURE4, GL_TEXTURE_2D);
}

}
