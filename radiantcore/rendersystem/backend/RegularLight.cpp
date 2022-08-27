#include "RegularLight.h"

#include "ishaders.h"
#include "OpenGLShader.h"
#include "ObjectRenderer.h"
#include "glprogram/DepthFillAlphaProgram.h"
#include "glprogram/ShadowMapProgram.h"

namespace render
{

RegularLight::RegularLight(RendererLight& light, IGeometryStore& store, IObjectRenderer& objectRenderer) :
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
    _isShadowCasting = _light.isShadowCasting() && _light.getShader() && 
        _light.getShader()->getMaterial() && _light.getShader()->getMaterial()->lightCastsShadows();
}

void RegularLight::addObject(IRenderableObject& object, IRenderEntity& entity, OpenGLShader* shader)
{
    auto& objectsByMaterial = _objectsByEntity.emplace(
        &entity, ObjectsByMaterial{}).first->second;

    auto& surfaces = objectsByMaterial.emplace(
        shader, ObjectList{}).first->second;

    surfaces.emplace_back(std::ref(object));

    ++_objectCount;
}

bool RegularLight::isInView(const IRenderView& view)
{
    return view.TestAABB(_lightBounds) != VOLUME_OUTSIDE;
}

bool RegularLight::isShadowCasting() const
{
    return _isShadowCasting;
}

void RegularLight::collectSurfaces(const IRenderView& view, const std::set<IRenderEntityPtr>& entities)
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

void RegularLight::fillDepthBuffer(OpenGLState& state, DepthFillAlphaProgram& program, 
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

void RegularLight::drawShadowMap(OpenGLState& state, const Rectangle& rectangle, 
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

RegularLight::InteractionDrawCall::InteractionDrawCall(OpenGLState& state, InteractionProgram& program,
    IObjectRenderer& objectRenderer, const Vector3& worldLightOrigin, const Vector3& viewer) :
    _state(state),
    _program(program),
    _objectRenderer(objectRenderer),
    _worldLightOrigin(worldLightOrigin),
    _viewer(viewer),
    _bump(nullptr),
    _diffuse(nullptr),
    _specular(nullptr),
    _interactionDrawCalls(0)
{
    _untransformedObjects.reserve(10000);
}

void RegularLight::InteractionDrawCall::submit(const ObjectList& objects)
{
    // Every material without bump defines an implicit _flat bump (see in TDM sources: Material::AddImplicitStages)
    if (!_bump)
    {
        _bump = &_defaultBumpStage;
    }

    // Substitute diffuse and specular with default images if necessary
    if (!_diffuse)
    {
        _diffuse = &_defaultDiffuseStage;
    }

    if (!_specular)
    {
        _specular = &_defaultSpecularStage;
    }

    // Bind textures
    OpenGLState::SetTextureState(_state.texture0, _diffuse->texture, GL_TEXTURE0, GL_TEXTURE_2D);
    OpenGLState::SetTextureState(_state.texture1, _bump->texture, GL_TEXTURE1, GL_TEXTURE_2D);
    OpenGLState::SetTextureState(_state.texture2, _specular->texture, GL_TEXTURE2, GL_TEXTURE_2D);

    // Enable alphatest if required
    if (_diffuse && _diffuse->stage && _diffuse->stage->hasAlphaTest())
    {
        glEnable(GL_ALPHA_TEST);
        glAlphaFunc(GL_GEQUAL, _diffuse->stage->getAlphaTest());
    }
    else
    {
        glDisable(GL_ALPHA_TEST);
    }

    // Load stage texture matrices
    _program.setDiffuseTextureTransform(_diffuse && _diffuse->stage ? _diffuse->stage->getTextureTransform() : Matrix4::getIdentity());
    _program.setBumpTextureTransform(_bump && _bump->stage ? _bump->stage->getTextureTransform() : Matrix4::getIdentity());
    _program.setSpecularTextureTransform(_specular && _specular->stage ? _specular->stage->getTextureTransform() : Matrix4::getIdentity());

    // Vertex colour mode and diffuse stage colour setup for this pass
    _program.setStageVertexColour(_diffuse && _diffuse->stage ? _diffuse->stage->getVertexColourMode() : IShaderLayer::VERTEX_COLOUR_NONE,
        _diffuse && _diffuse->stage ? _diffuse->stage->getColour() : Colour4::WHITE());

    for (const auto& object : objects)
    {
        // We submit all objects with an identity matrix in a single multi draw call
        if (!object.get().isOriented())
        {
            _untransformedObjects.push_back(object.get().getStorageLocation());
            continue;
        }

        _program.setUpObjectLighting(_worldLightOrigin, _viewer, object.get().getObjectTransform().getInverse());
        _program.setObjectTransform(object.get().getObjectTransform());

        _objectRenderer.submitGeometry(object.get().getStorageLocation(), GL_TRIANGLES);
        ++_interactionDrawCalls;
    }

    if (!_untransformedObjects.empty())
    {
        _program.setUpObjectLighting(_worldLightOrigin, _viewer, Matrix4::getIdentity());
        _program.setObjectTransform(Matrix4::getIdentity());

        _objectRenderer.submitGeometry(_untransformedObjects, GL_TRIANGLES);
        ++_interactionDrawCalls;

        _untransformedObjects.clear();
    }
}

void RegularLight::InteractionDrawCall::setBump(const InteractionPass::Stage* bump)
{
    _bump = bump;
}

void RegularLight::InteractionDrawCall::setDiffuse(const InteractionPass::Stage* diffuse)
{
    _diffuse = diffuse;
}

void RegularLight::InteractionDrawCall::setSpecular(const InteractionPass::Stage* specular)
{
    _specular = specular;
}

void RegularLight::drawInteractions(OpenGLState& state, InteractionProgram& program, 
    const IRenderView& view, std::size_t renderTime)
{
    if (_objectsByEntity.empty())
    {
        return;
    }

    auto worldLightOrigin = _light.getLightOrigin();

    InteractionDrawCall draw(state, program, _objectRenderer, worldLightOrigin, view.getViewer());

    // Set up textures used by this light
    program.setupLightParameters(state, _light, renderTime);

    for (const auto& [entity, objectsByShader] : _objectsByEntity)
    {
        for (const auto& [shader, objects] : objectsByShader)
        {
            const auto pass = shader->getInteractionPass();

            if (!pass) continue;

            draw.prepare(*pass);

            for (const auto& interactionStage : pass->getInteractionStages())
            {
                interactionStage.stage->evaluateExpressions(renderTime, *entity);

                if (!interactionStage.stage->isVisible()) continue; // ignore inactive stages

                switch (interactionStage.stage->getType())
                {
                case IShaderLayer::BUMP:
                    if (draw.hasBump())
                    {
                        draw.submit(objects); // submit pending draws when changing bump maps
                    }
                    draw.setBump(&interactionStage);
                    break;
                case IShaderLayer::DIFFUSE:
                    if (draw.hasDiffuse())
                    {
                        draw.submit(objects); // submit pending draws when changing diffuse maps
                    }
                    draw.setDiffuse(&interactionStage);
                    break;
                case IShaderLayer::SPECULAR:
                    if (draw.hasSpecular())
                    {
                        draw.submit(objects); // submit pending draws when changing specular maps
                    }
                    draw.setSpecular(&interactionStage);
                    break;
                default:
                    throw std::logic_error("Non-interaction stage encountered in interaction pass");
                }
            }

            // Submit the pending draw call
            draw.submit(objects);
        }
    }

    _interactionDrawCalls += draw.getInteractionDrawCalls();

    // Unbind the light textures
    OpenGLState::SetTextureState(state.texture3, 0, GL_TEXTURE3, GL_TEXTURE_2D);
    OpenGLState::SetTextureState(state.texture4, 0, GL_TEXTURE4, GL_TEXTURE_2D);
}

void RegularLight::setupAlphaTest(OpenGLState& state, OpenGLShader* shader, DepthFillPass* depthFillPass,
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
