#include "ColourShader.h"

#include <stdexcept>
#include "string/convert.h"
#include "fmt/format.h"

namespace render
{

ColourShader::ColourShader(ColourShaderType type, const Colour4& colour, OpenGLRenderSystem& renderSystem) :
    OpenGLShader(ConstructName(type, colour), renderSystem),
    _type(type),
    _colour(colour)
{}

void ColourShader::construct()
{
    switch (_type)
    {
    case ColourShaderType::CameraSolid:
    {
        OpenGLState& state = appendDefaultPass();
        state.setName(getName());

        state.setColour(
            static_cast<float>(_colour.x()),
            static_cast<float>(_colour.y()),
            static_cast<float>(_colour.z()),
            1.0f
        );

        state.setRenderFlag(RENDER_FILL);
        state.setRenderFlag(RENDER_LIGHTING);
        state.setRenderFlag(RENDER_DEPTHTEST);
        state.setRenderFlag(RENDER_CULLFACE);
        state.setRenderFlag(RENDER_DEPTHWRITE);
        state.setSortPosition(OpenGLState::SORT_FULLBRIGHT);

        enableViewType(RenderViewType::Camera);
        break;
    }

    case ColourShaderType::CameraTranslucent:
    {
        OpenGLState& state = appendDefaultPass();
        state.setName(getName());

        state.setColour(
            static_cast<float>(_colour.x()),
            static_cast<float>(_colour.y()),
            static_cast<float>(_colour.z()),
            0.5f
        );

        state.setRenderFlag(RENDER_FILL);
        state.setRenderFlag(RENDER_LIGHTING);
        state.setRenderFlag(RENDER_DEPTHTEST);
        state.setRenderFlag(RENDER_CULLFACE);
        state.setRenderFlag(RENDER_DEPTHWRITE);
        state.setRenderFlag(RENDER_BLEND);
        state.setSortPosition(OpenGLState::SORT_TRANSLUCENT);

        enableViewType(RenderViewType::Camera);
        break;
    }

    case ColourShaderType::OrthoviewSolid:
    {
        // Wireframe renderer is using GL_LINES to display each winding
        setWindingRenderer(std::make_unique<WindingRenderer<WindingIndexer_Lines>>());

        OpenGLState& state = appendDefaultPass();
        state.setName(getName());

        state.setColour(
            static_cast<float>(_colour.x()),
            static_cast<float>(_colour.y()),
            static_cast<float>(_colour.z()),
            1.0f
        );

        state.setRenderFlags(RENDER_DEPTHTEST | RENDER_DEPTHWRITE);
        state.setSortPosition(OpenGLState::SORT_FULLBRIGHT);
        state.setDepthFunc(GL_LESS);
        state.m_linewidth = 1;
        state.m_pointsize = 1;

        enableViewType(RenderViewType::OrthoView);
        break;
    }

    case ColourShaderType::CameraAndOrthoview:
    {
        OpenGLState& state = appendDefaultPass();
        state.setName(getName());

        state.setColour(
            static_cast<float>(_colour.x()),
            static_cast<float>(_colour.y()),
            static_cast<float>(_colour.z()),
            1.0f
        );

        state.setRenderFlag(RENDER_FILL);
        state.setRenderFlag(RENDER_LIGHTING);
        state.setRenderFlag(RENDER_DEPTHTEST);
        state.setRenderFlag(RENDER_CULLFACE);
        state.setRenderFlag(RENDER_DEPTHWRITE);
        state.setRenderFlag(RENDER_BLEND);
        state.setSortPosition(OpenGLState::SORT_TRANSLUCENT);
        state.setDepthFunc(GL_LESS);
        state.m_linewidth = 1;
        state.m_pointsize = 1;

        // Applicable to both views
        enableViewType(RenderViewType::OrthoView);
        enableViewType(RenderViewType::Camera);
        break;
    }

    default:
        throw std::runtime_error("Cannot construct colour shader type: " + string::to_string(static_cast<int>(_type)));
    }
}

std::string ColourShader::ConstructName(ColourShaderType type, const Colour4& colour)
{
    switch (type)
    {
    case ColourShaderType::CameraSolid:
        return fmt::format("({0:f} {1:f} {2:f})", colour[0], colour[1], colour[2]);
        break;

    case ColourShaderType::CameraTranslucent:
        return fmt::format("[{0:f} {1:f} {2:f}]", colour[0], colour[1], colour[2]);

    case ColourShaderType::OrthoviewSolid:
        return fmt::format("<{0:f} {1:f} {2:f}>", colour[0], colour[1], colour[2]);

    case ColourShaderType::CameraAndOrthoview:
        return fmt::format("{{{0:f} {1:f} {2:f}}}", colour[0], colour[1], colour[2]);
    }

    throw std::runtime_error("Unknown colour shader type: " + string::to_string(static_cast<int>(type)));
}

}
