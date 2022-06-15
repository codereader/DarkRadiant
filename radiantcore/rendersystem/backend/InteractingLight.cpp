#include "InteractingLight.h"

#include "OpenGLShader.h"
#include "ObjectRenderer.h"
#include "glprogram/DepthFillAlphaProgram.h"
#include "glprogram/ShadowMapProgram.h"

namespace render
{

InteractingLight::InteractingLight(RendererLight& light, IGeometryStore& store, IObjectRenderer& objectRenderer) :
    _light(light),
    _store(store),
    _objectRenderer(objectRenderer),
    _lightBounds(light.lightAABB()),
    _interactionDrawCalls(0),
    _depthDrawCalls(0),
    _objectCount(0),
    _shadowMapDrawCalls(0),
    _shadowLightIndex(-1)
{
    // Consider the "noshadows" flag and the setting of the light material
    _isShadowCasting = _light.isShadowCasting() && _light.getShader() && _light.getShader()->getMaterial()->lightCastsShadows();
}

void InteractingLight::addObject(IRenderableObject& object, IRenderEntity& entity, OpenGLShader* shader)
{
    auto& objectsByMaterial = _objectsByEntity.emplace(
        &entity, ObjectsByMaterial{}).first->second;

    auto& surfaces = objectsByMaterial.emplace(
        shader, ObjectList{}).first->second;

    surfaces.emplace_back(std::ref(object));

    ++_objectCount;
}

bool InteractingLight::isInView(const IRenderView& view)
{
    return view.TestAABB(_lightBounds) != VOLUME_OUTSIDE;
}

bool InteractingLight::isShadowCasting() const
{
    return _isShadowCasting;
}

void InteractingLight::collectSurfaces(const IRenderView& view, const std::set<IRenderEntityPtr>& entities)
{
    bool shadowCasting = isShadowCasting();

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

            // For non-shadow lights we can cull surfaces that are not in view
            if (!shadowCasting)
            {
                if (object->isOriented())
                {
                    if (view.TestAABB(object->getObjectBounds(), object->getObjectTransform()) == VOLUME_OUTSIDE)
                    {
                        return;
                    }
                }
                else if (view.TestAABB(object->getObjectBounds()) == VOLUME_OUTSIDE) // non-oriented AABB test
                {
                    return;
                }
            }

            auto glShader = static_cast<OpenGLShader*>(shader);

            // We only consider materials designated for camera rendering
            if (!glShader->isApplicableTo(RenderViewType::Camera))
            {
                return;
            }

            // Collect all interaction surfaces and the ones with forceShadows materials
            if (!glShader->getInteractionPass() && (!shader->getMaterial() || !shader->getMaterial()->surfaceCastsShadow()))
            {
                return; // This material doesn't interact with this light
            }

            addObject(*object, *entity, glShader);
        });
    }
}

void InteractingLight::fillDepthBuffer(OpenGLState& state, DepthFillAlphaProgram& program, 
    std::size_t renderTime, std::vector<IGeometryStore::Slot>& untransformedObjectsWithoutAlphaTest)
{
    std::vector<IGeometryStore::Slot> untransformedObjects;
    untransformedObjects.reserve(1000);

    for (const auto& [entity, objectsByShader] : _objectsByEntity)
    {
        for (const auto& [shader, objects] : objectsByShader)
        {
            auto depthFillPass = shader->getDepthFillPass();

            if (!depthFillPass) continue;

            setupAlphaTest(state, shader, depthFillPass, program, renderTime, entity);

            for (const auto& object : objects)
            {
                // We submit all objects with an identity matrix in a single multi draw call
                if (!object.get().isOriented())
                {
                    if (shader->getMaterial()->getCoverage() == Material::MC_PERFORATED)
                    {
                        untransformedObjects.push_back(object.get().getStorageLocation());
                    }
                    else
                    {
                        // Put it on the huge pile of non-alphatest materials
                        untransformedObjectsWithoutAlphaTest.push_back(object.get().getStorageLocation());
                    }

                    continue;
                }

                program.setObjectTransform(object.get().getObjectTransform());

                _objectRenderer.submitGeometry(object.get().getStorageLocation(), GL_TRIANGLES);
                ++_depthDrawCalls;
            }

            // All alpha-tested materials without transform need to be submitted now
            if (!untransformedObjects.empty())
            {
                program.setObjectTransform(Matrix4::getIdentity());

                _objectRenderer.submitGeometry(untransformedObjects, GL_TRIANGLES);
                ++_depthDrawCalls;

                untransformedObjects.clear();
            }
        }
    }
}

void InteractingLight::drawShadowMap(OpenGLState& state, const Rectangle& rectangle, 
    ShadowMapProgram& program, std::size_t renderTime)
{
    // Set up the viewport to write to a specific area within the shadow map texture
    glViewport(rectangle.x, rectangle.y, 6 * rectangle.width, rectangle.width);

    std::vector<IGeometryStore::Slot> untransformedObjects;
    untransformedObjects.reserve(1000);

    program.setLightOrigin(_light.getLightOrigin());

    // Set evaluated stage texture transformation matrix to the GLSL uniform
    program.setDiffuseTextureTransform(Matrix4::getIdentity());

    // Render all the objects that have a depth filling stage
    for (const auto& [entity, objectsByShader] : _objectsByEntity)
    {
        if (!entity->isShadowCasting()) continue; // skip all entities with "noshadows" set

        for (const auto& [shader, objects] : objectsByShader)
        {
            const auto& material = shader->getMaterial();

            // Skip materials not casting any shadow. This includes all
            // translucent materials, they get the noshadows flag set implicitly
            if (!material->surfaceCastsShadow()) continue;

            // Set up alphatest (it's ok to pass a nullptr as depth fill pass)
            setupAlphaTest(state, shader, shader->getDepthFillPass(), program, renderTime, entity);

            for (const auto& object : objects)
            {
                // Skip models with "noshadows" set (this might be redundant to the entity check above)
                if (!object.get().isShadowCasting()) continue;

                // We submit all objects with an identity matrix in a single multi draw call
                if (!object.get().isOriented())
                {
                    untransformedObjects.push_back(object.get().getStorageLocation());
                    continue;
                }

                program.setObjectTransform(object.get().getObjectTransform());

                _objectRenderer.submitInstancedGeometry(object.get().getStorageLocation(), 6, GL_TRIANGLES);
                ++_shadowMapDrawCalls;
            }

            if (!untransformedObjects.empty())
            {
                program.setObjectTransform(Matrix4::getIdentity());

                _objectRenderer.submitInstancedGeometry(untransformedObjects, 6, GL_TRIANGLES);
                ++_shadowMapDrawCalls;

                untransformedObjects.clear();
            }
        }
    }

    debug::assertNoGlErrors();
}

void InteractingLight::drawInteractions(OpenGLState& state, InteractionProgram& program, 
    const IRenderView& view, std::size_t renderTime)
{
    if (_objectsByEntity.empty())
    {
        return;
    }

    auto worldLightOrigin = _light.getLightOrigin();

    std::vector<IGeometryStore::Slot> untransformedObjects;
    untransformedObjects.reserve(10000);

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

                program.setUpObjectLighting(worldLightOrigin, view.getViewer(), 
                    object.get().getObjectTransform().getInverse());

                pass->getProgram().setObjectTransform(object.get().getObjectTransform());

                _objectRenderer.submitGeometry(object.get().getStorageLocation(), GL_TRIANGLES);
                ++_interactionDrawCalls;
            }

            if (!untransformedObjects.empty())
            {
                program.setUpObjectLighting(worldLightOrigin, view.getViewer(), Matrix4::getIdentity());

                pass->getProgram().setObjectTransform(Matrix4::getIdentity());

                _objectRenderer.submitGeometry(untransformedObjects, GL_TRIANGLES);
                ++_interactionDrawCalls;

                untransformedObjects.clear();
            }
        }
    }

    // Unbind the light textures
    OpenGLState::SetTextureState(state.texture3, 0, GL_TEXTURE3, GL_TEXTURE_2D);
    OpenGLState::SetTextureState(state.texture4, 0, GL_TEXTURE4, GL_TEXTURE_2D);
}

void InteractingLight::setupAlphaTest(OpenGLState& state, OpenGLShader* shader, DepthFillPass* depthFillPass,
    ISupportsAlphaTest& program, std::size_t renderTime, IRenderEntity* entity)
{
    const auto& material = shader->getMaterial();
    assert(material);

    auto coverage = material->getCoverage();

    // Skip translucent materials
    if (coverage == Material::MC_TRANSLUCENT) return;

    if (coverage == Material::MC_PERFORATED && depthFillPass != nullptr)
    {
        // Evaluate the shader stages of this material
        depthFillPass->evaluateShaderStages(renderTime, entity);

        // Apply the alpha test value, it might be affected by time and entity parms
        program.setAlphaTest(depthFillPass->getAlphaTestValue());

        // If there's a diffuse stage, apply the correct texture
        OpenGLState::SetTextureState(state.texture0, depthFillPass->state().texture0, GL_TEXTURE0, GL_TEXTURE_2D);

        // Set evaluated stage texture transformation matrix to the GLSL uniform
        program.setDiffuseTextureTransform(depthFillPass->getDiffuseTextureTransform());
    }
    else
    {
        // No alpha test on this material, pass -1 to deactivate texture sampling
        // in the GLSL program
        program.setAlphaTest(-1);
    }
}

}
