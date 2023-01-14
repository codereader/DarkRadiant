#pragma once

#include "OpenGLShaderPass.h"
#include "glprogram/InteractionProgram.h"

namespace render
{

class OpenGLRenderSystem;
class GLProgramFactory;

/**
 * Lighting Interaction pass (Diffuse/Bump/Specular).
 */
class InteractionPass :
    public OpenGLShaderPass
{
public:
    // An interaction stage prepared for rendering
    struct Stage
    {
        IShaderLayer::Ptr stage;
        GLuint texture;
    };

private:
    std::vector<Stage> _interactionStages;

    GLuint _defaultDiffuseTexture;
    GLuint _defaultBumpTexture;
    GLuint _defaultSpecularTexture;

public:
    InteractionPass(OpenGLShader& owner, OpenGLRenderSystem& renderSystem, std::vector<IShaderLayer::Ptr>& stages);

    InteractionProgram& getProgram()
    {
        return *static_cast<InteractionProgram*>(_glState.glProgram);
    }

    Matrix4 getDiffuseTextureTransform() const
    {
        return GetTextureTransformFromStage(state().stage0);
    }

    Matrix4 getBumpTextureTransform() const
    {
        return GetTextureTransformFromStage(state().stage1);
    }

    Matrix4 getSpecularTextureTransform() const
    {
        return GetTextureTransformFromStage(state().stage2);
    }

    const std::vector<Stage>& getInteractionStages() const
    {
        return _interactionStages;
    }

    GLuint getDefaultInteractionTextureBinding(IShaderLayer::Type type);

    // Generates the state with all the required flags for drawing interaction passes
    static OpenGLState GenerateInteractionState(GLProgramFactory& programFactory);

private:
    inline static Matrix4 GetTextureTransformFromStage(const IShaderLayer::Ptr& stage)
    {
        return stage ? stage->getTextureTransform() : Matrix4::getIdentity();
    }
};

}
