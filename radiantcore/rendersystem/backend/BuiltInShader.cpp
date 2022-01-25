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

    case BuiltInShaderType::AasAreaBounds:
    {
        pass.setColour(1, 1, 1, 1);
        pass.setRenderFlags(RENDER_DEPTHWRITE
            | RENDER_DEPTHTEST
            | RENDER_OVERRIDE);
        pass.setSortPosition(OpenGLState::SORT_OVERLAY_LAST);
        pass.setDepthFunc(GL_LEQUAL);

        OpenGLState& hiddenLine = appendDefaultPass();
        hiddenLine.setColour(1, 1, 1, 1);
        hiddenLine.setRenderFlags(RENDER_DEPTHWRITE
            | RENDER_DEPTHTEST
            | RENDER_OVERRIDE
            | RENDER_LINESTIPPLE);
        hiddenLine.setSortPosition(OpenGLState::SORT_OVERLAY_LAST);
        hiddenLine.setDepthFunc(GL_GREATER);

        enableViewType(RenderViewType::Camera);
        break;
    }

    case BuiltInShaderType::MissingModel:
    {
        // Render a custom texture
        auto imgPath = module::GlobalModuleRegistry().getApplicationContext().getBitmapsPath();
        imgPath += "missing_model.tga";

        auto editorTex = GlobalMaterialManager().loadTextureFromFile(imgPath);
        pass.texture0 = editorTex ? editorTex->getGLTexNum() : 0;

        pass.setRenderFlag(RENDER_FILL);
        pass.setRenderFlag(RENDER_TEXTURE_2D);
        pass.setRenderFlag(RENDER_DEPTHTEST);
        pass.setRenderFlag(RENDER_LIGHTING);
        pass.setRenderFlag(RENDER_SMOOTH);
        pass.setRenderFlag(RENDER_DEPTHWRITE);
        pass.setRenderFlag(RENDER_CULLFACE);

        // Set the GL color to white
        pass.setColour(Colour4::WHITE());
        pass.setSortPosition(OpenGLState::SORT_FULLBRIGHT);

        enableViewType(RenderViewType::Camera);
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
