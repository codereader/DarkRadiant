#pragma once

#include <map>
#include <vector>
#include "irender.h"
#include "isurfacerenderer.h"
#include "irenderview.h"

namespace render
{

/**
 * Defines an interaction between a light and one or more entity surfaces
 * It only lives through the course of a single render pass, therefore direct
 * references without ref-counting are used.
 * 
 * Surfaces are grouped by entity, then by shader.
 */
class LightInteractionList
{
private:
    RendererLight& _light;

    // A flat list of surfaces
    using SurfaceList = std::vector<std::reference_wrapper<IRenderableSurface>>;

    // All surfaces, grouped by material
    using SurfacesByMaterial = std::map<OpenGLShader*, SurfaceList>;

    // SurfaceLists, grouped by entity
    std::map<IRenderEntity*, SurfacesByMaterial> _surfacesByEntity;

public:
    LightInteractionList(RendererLight& light) :
        _light(light)
    {}

    void addSurface(IRenderableSurface& surface, IRenderEntity& entity, OpenGLShader& shader)
    {
        auto& surfacesByMaterial = _surfacesByEntity.emplace(
            &entity, SurfacesByMaterial{}).first->second;

        auto& surfaces = surfacesByMaterial.emplace(
            &shader, SurfaceList{}).first->second;
        
        surfaces.emplace_back(std::ref(surface));
    }

    void render(OpenGLState& state, RenderStateFlags globalFlagsMask, const IRenderView& view, std::size_t renderTime)
    {
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glEnableClientState(GL_NORMAL_ARRAY);

        // Render surfaces without any vertex colours(?)
        glDisableClientState(GL_COLOR_ARRAY);

        glFrontFace(GL_CCW);

        for (auto& pair : _surfacesByEntity)
        {
            auto entity = pair.first;

            for (auto& pair : pair.second)
            {
                auto shader = pair.first;
                auto& surfaceList = pair.second;

                if (!shader->isVisible()) continue;

                shader->foreachPass([&](OpenGLShaderPass& pass)
                {
                    // Reset the texture matrix
                    glMatrixMode(GL_TEXTURE);
                    glLoadMatrixd(Matrix4::getIdentity());

                    glMatrixMode(GL_MODELVIEW);

                    // Apply our state to the current state object
                    pass.applyState(state, globalFlagsMask, view.getViewer(), renderTime, entity);

                    RenderInfo info(state.getRenderFlags(), view.getViewer(), state.cubeMapMode);
                    
                    for (auto surface : surfaceList)
                    {
                        if (state.glProgram)
                        {
                            OpenGLShaderPass::setUpLightingCalculation(state, &_light,
                                view.getViewer(), surface.get().getSurfaceTransform(), renderTime, state.isColourInverted());
                        }

                        if (surface.get().getSurfaceTransform().getHandedness() == Matrix4::RIGHTHANDED)
                        {
                            glFrontFace(GL_CW);
                        }
                        else
                        {
                            glFrontFace(GL_CCW);
                        }

                        glMatrixMode(GL_MODELVIEW);
                        glPushMatrix();

                        glMultMatrixd(surface.get().getSurfaceTransform());

                        const auto& vertices = surface.get().getVertices();
                        const auto& indices = surface.get().getIndices();

                        glVertexPointer(3, GL_DOUBLE, sizeof(ArbitraryMeshVertex), &vertices.front().vertex);
                        glColorPointer(4, GL_DOUBLE, sizeof(ArbitraryMeshVertex), &vertices.front().colour);

                        glVertexAttribPointer(ATTR_NORMAL, 3, GL_DOUBLE, 0, sizeof(ArbitraryMeshVertex), &vertices.front().normal);
                        glVertexAttribPointer(ATTR_TEXCOORD, 2, GL_DOUBLE, 0, sizeof(ArbitraryMeshVertex), &vertices.front().texcoord);
                        glVertexAttribPointer(ATTR_TANGENT, 3, GL_DOUBLE, 0, sizeof(ArbitraryMeshVertex), &vertices.front().tangent);
                        glVertexAttribPointer(ATTR_BITANGENT, 3, GL_DOUBLE, 0, sizeof(ArbitraryMeshVertex), &vertices.front().bitangent);

                        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, &indices.front());

                        glPopMatrix();
                    }
                });
            }
        }

        glDisableClientState(GL_VERTEX_ARRAY);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        glDisableClientState(GL_NORMAL_ARRAY);
    }
};

}
