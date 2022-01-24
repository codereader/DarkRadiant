#include "BuiltInShader.h"

#include "string/convert.h"

namespace render
{

BuiltInShader::BuiltInShader(BuiltInShaderType type, OpenGLRenderSystem& renderSystem) :
    OpenGLShader(GetNameForType(type), renderSystem),
    _type(type)
{}

void BuiltInShader::construct()
{
    auto& pass = appendDefaultPass();
    pass.setName(getName());

    switch (_type)
    {
    case BuiltInShaderType::FlatshadeOverlay:
    {
        pass.setRenderFlags(RENDER_CULLFACE
            | RENDER_LIGHTING
            | RENDER_SMOOTH
            | RENDER_SCALED
            | RENDER_FILL
            | RENDER_DEPTHWRITE
            | RENDER_DEPTHTEST
            | RENDER_OVERRIDE);
        pass.setSortPosition(OpenGLState::SORT_GUI1);
        pass.setDepthFunc(GL_LEQUAL);

        OpenGLState& hiddenLine = appendDefaultPass();
        hiddenLine.setName(getName() + "_Hidden");
        hiddenLine.setRenderFlags(RENDER_CULLFACE
            | RENDER_LIGHTING
            | RENDER_SMOOTH
            | RENDER_SCALED
            | RENDER_FILL
            | RENDER_DEPTHWRITE
            | RENDER_DEPTHTEST
            | RENDER_OVERRIDE
            | RENDER_POLYGONSTIPPLE);
        hiddenLine.setSortPosition(OpenGLState::SORT_GUI0);
        hiddenLine.setDepthFunc(GL_GREATER);

        enableViewType(RenderViewType::Camera);
        enableViewType(RenderViewType::OrthoView);
        break;
    }
    default:
        throw std::runtime_error("Cannot construct this shader: " + getName());
    }
}

std::string BuiltInShader::GetNameForType(BuiltInShaderType type)
{
    return "$BUILT_IN_SHADER[" + string::to_string(static_cast<std::size_t>(type)) + "]";
}

}
