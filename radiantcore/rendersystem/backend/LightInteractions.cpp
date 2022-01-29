#include "LightInteractions.h"

#include "OpenGLShader.h"

namespace render
{

namespace detail
{

inline void submitObject(IRenderableObject& object)
{
    if (object.getObjectTransform().getHandedness() == Matrix4::RIGHTHANDED)
    {
        glFrontFace(GL_CW);
    }
    else
    {
        glFrontFace(GL_CCW);
    }

    glMatrixMode(GL_MODELVIEW);

    auto surface = dynamic_cast<IRenderableSurface*>(&object);

    if (surface)
    {
        glPushMatrix();

        glMultMatrixd(surface->getObjectTransform());

        const auto& vertices = surface->getVertices();
        const auto& indices = surface->getIndices();

        glVertexPointer(3, GL_DOUBLE, sizeof(ArbitraryMeshVertex), &vertices.front().vertex);

        glVertexAttribPointer(ATTR_NORMAL, 3, GL_DOUBLE, 0, sizeof(ArbitraryMeshVertex), &vertices.front().normal);
        glVertexAttribPointer(ATTR_TEXCOORD, 2, GL_DOUBLE, 0, sizeof(ArbitraryMeshVertex), &vertices.front().texcoord);
        glVertexAttribPointer(ATTR_TANGENT, 3, GL_DOUBLE, 0, sizeof(ArbitraryMeshVertex), &vertices.front().tangent);
        glVertexAttribPointer(ATTR_BITANGENT, 3, GL_DOUBLE, 0, sizeof(ArbitraryMeshVertex), &vertices.front().bitangent);

        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, &indices.front());
        
        glPopMatrix();
        return;
    }

    // TODO
}

}

void LightInteractions::addObject(IRenderableObject& object, IRenderEntity& entity, OpenGLShader& shader)
{
    auto& objectsByMaterial = _objectsByEntity.emplace(
        &entity, ObjectsByMaterial{}).first->second;

    auto& surfaces = objectsByMaterial.emplace(
        &shader, ObjectList{}).first->second;

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
            [&](const render::IRenderableObject::Ptr& object, const ShaderPtr& shader)
        {
            // Skip empty objects
            if (!object->isVisible()) return;

            // Don't collect invisible shaders
            if (!shader->isVisible()) return;

            auto glShader = static_cast<OpenGLShader*>(shader.get());

            // We only consider materials designated for camera rendering
            if (!glShader->isApplicableTo(RenderViewType::Camera))
            {
                return;
            }

            addObject(*object, *entity, *glShader);
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

    for (auto& pair : _objectsByEntity)
    {
        auto entity = pair.first;

        for (auto& pair : pair.second)
        {
            auto shader = pair.first;
            auto& objectList = pair.second;

            // Skip translucent materials
            if (shader->getMaterial() && shader->getMaterial()->getCoverage() == Material::MC_TRANSLUCENT)
            {
                continue;
            }

            if (!shader->getDepthFillPass()) continue;

            // Reset the texture matrix
            glMatrixMode(GL_TEXTURE);
            glLoadMatrixd(Matrix4::getIdentity());

            glMatrixMode(GL_MODELVIEW);

            // Apply our state to the current state object
            shader->getDepthFillPass()->applyState(state, globalFlagsMask, view.getViewer(), renderTime, entity);

            RenderInfo info(state.getRenderFlags(), view.getViewer(), state.cubeMapMode);
            
            for (auto object : objectList)
            {
                detail::submitObject(object.get());
                ++_drawCalls;
            }
        }
    }

    glDisableClientState(GL_VERTEX_ARRAY);
}

void LightInteractions::render(OpenGLState& state, RenderStateFlags globalFlagsMask, const IRenderView& view, std::size_t renderTime)
{
    auto worldToLight = _light.getLightTextureTransformation();

    glEnableClientState(GL_VERTEX_ARRAY);

    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);

    for (auto& pair : _objectsByEntity)
    {
        auto entity = pair.first;

        for (auto& pair : pair.second)
        {
            auto shader = pair.first;
            auto& objectList = pair.second;

            if (!shader->isVisible()) continue;

            shader->foreachPassWithoutDepthPass([&](OpenGLShaderPass& pass)
            {
                if (!pass.stateIsActive())
                {
                    return;
                }

                // Reset the texture matrix
                glMatrixMode(GL_TEXTURE);
                glLoadMatrixd(Matrix4::getIdentity());

                glMatrixMode(GL_MODELVIEW);

                // Apply our state to the current state object
                pass.applyState(state, globalFlagsMask, view.getViewer(), renderTime, entity);

                RenderInfo info(state.getRenderFlags(), view.getViewer(), state.cubeMapMode);

                for (auto object : objectList)
                {
                    if (state.glProgram)
                    {
                        OpenGLShaderPass::setUpLightingCalculation(state, &_light, worldToLight,
                            view.getViewer(), object.get().getObjectTransform(), renderTime, state.isColourInverted());
                    }

                    detail::submitObject(object.get());
                    ++_drawCalls;
                }
            });
        }
    }

    glDisableClientState(GL_VERTEX_ARRAY);
}

}
