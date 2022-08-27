#include "InteractionPass.h"

#include "GLProgramFactory.h"
#include "../OpenGLRenderSystem.h"
#include "glprogram/InteractionProgram.h"

namespace render
{

namespace
{
    TexturePtr getDefaultInteractionTexture(IShaderLayer::Type type)
    {
        return GlobalMaterialManager().getDefaultInteractionTexture(type);
    }

    TexturePtr getTextureOrInteractionDefault(const IShaderLayer::Ptr& layer)
    {
        auto texture = layer->getTexture();
        return texture ? texture : getDefaultInteractionTexture(layer->getType());
    }
}

inline void setInteractionStateFlags(OpenGLState& state, GLProgramFactory& programFactory)
{
    // Set render flags
    state.setRenderFlag(RENDER_BLEND);
    state.setRenderFlag(RENDER_FILL);
    state.setRenderFlag(RENDER_TEXTURE_2D);
    state.setRenderFlag(RENDER_CULLFACE);
    state.setRenderFlag(RENDER_DEPTHTEST);
    state.setRenderFlag(RENDER_SMOOTH);
    state.setRenderFlag(RENDER_BUMP);
    state.setRenderFlag(RENDER_PROGRAM);

    state.glProgram = programFactory.getBuiltInProgram(ShaderProgram::Interaction);
    assert(dynamic_cast<InteractionProgram*>(state.glProgram));

    state.setDepthFunc(GL_LEQUAL);
    state.setSortPosition(OpenGLState::SORT_INTERACTION);
    state.m_blend_src = GL_ONE;
    state.m_blend_dst = GL_ONE;
}

InteractionPass::InteractionPass(OpenGLShader& owner, OpenGLRenderSystem& renderSystem, std::vector<IShaderLayer::Ptr>& stages) :
    OpenGLShaderPass(owner)
{
    setInteractionStateFlags(_glState, renderSystem.getGLProgramFactory());

    // Load the textures or fall back to interaction defaults, but don't leave them empty
    _interactionStages.reserve(stages.size());

    for (auto&& stage : stages)
    {
        auto texture = getTextureOrInteractionDefault(stage)->getGLTexNum();
        _interactionStages.emplace_back(Stage{ std::move(stage), texture });
    }

    _defaultBumpTexture = getDefaultInteractionTexture(IShaderLayer::BUMP)->getGLTexNum();
    _defaultDiffuseTexture = getDefaultInteractionTexture(IShaderLayer::DIFFUSE)->getGLTexNum();
    _defaultSpecularTexture = getDefaultInteractionTexture(IShaderLayer::SPECULAR)->getGLTexNum();
}

GLuint InteractionPass::getDefaultInteractionTextureBinding(IShaderLayer::Type type)
{
    switch (type)
    {
    case IShaderLayer::DIFFUSE: return _defaultDiffuseTexture;
    case IShaderLayer::BUMP: return _defaultBumpTexture;
    case IShaderLayer::SPECULAR: return _defaultSpecularTexture;
    default: throw std::invalid_argument("Non-interaction default texture requested");
    }
}

OpenGLState InteractionPass::GenerateInteractionState(GLProgramFactory& programFactory)
{
    OpenGLState state;

    setInteractionStateFlags(state, programFactory);

    return state;
}

}
