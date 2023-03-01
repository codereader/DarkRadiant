#include "ColourShader.h"

#include <stdexcept>
#include "string/convert.h"
#include "fmt/format.h"
#include "../OpenGLRenderSystem.h"

namespace render
{

ColourShader::ColourShader(ColourShaderType type, const Colour4& colour, OpenGLRenderSystem& renderSystem) :
    OpenGLShader(ConstructName(type, colour), renderSystem),
    _type(type),
    _colour(colour)
{}

ColourShaderType ColourShader::getType() const
{
    return _type;
}

void ColourShader::construct()
{
    OpenGLState& state = appendDefaultPass();
    state.setName(getName());

    // Set the colour to non-transparent by default
    state.setColour(
        static_cast<float>(_colour.x()),
        static_cast<float>(_colour.y()),
        static_cast<float>(_colour.z()),
        1.0f
    );

    switch (_type)
    {
    case ColourShaderType::CameraOutline:
    case ColourShaderType::CameraSolid:
    {
        if (_type == ColourShaderType::CameraSolid)
        {
            state.setRenderFlag(RENDER_FILL);
        }
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
        // Don't touch a renderer that is not empty, this will break any client connections
        if (getWindingRenderer().empty())
        {
            setWindingRenderer(std::make_unique<WindingRenderer<WindingIndexer_Lines>>(getRenderSystem().getGeometryStore(),
                getRenderSystem().getObjectRenderer(), this));
        }

        state.setRenderFlags(RENDER_DEPTHTEST | RENDER_DEPTHWRITE);

        if (isMergeModeEnabled())
        {
            // merge mode, switch to transparent grey rendering
            state.setColour({ 0, 0, 0, 0.05f });

            state.m_blend_src = GL_SRC_ALPHA;
            state.m_blend_dst = GL_ONE_MINUS_SRC_ALPHA;

            state.setRenderFlags(RENDER_BLEND);
        }

        state.setSortPosition(OpenGLState::SORT_FULLBRIGHT);
        state.setDepthFunc(GL_LESS);
        state.lineWidth = 1;
        state.m_pointsize = 1;

        enableViewType(RenderViewType::OrthoView);
        break;
    }

    case ColourShaderType::CameraAndOrthoview:
    case ColourShaderType::CameraAndOrthoViewOutline:
    {
        if (_type == ColourShaderType::CameraAndOrthoview)
        {
            state.setRenderFlag(RENDER_FILL);
        }

        state.setRenderFlag(RENDER_LIGHTING);
        state.setRenderFlag(RENDER_DEPTHTEST);
        state.setRenderFlag(RENDER_CULLFACE);
        state.setRenderFlag(RENDER_DEPTHWRITE);
        state.setRenderFlag(RENDER_BLEND);
        state.setSortPosition(OpenGLState::SORT_TRANSLUCENT);
        state.setDepthFunc(GL_LESS);
        state.lineWidth = 1;
        state.m_pointsize = 1;

        if (isMergeModeEnabled())
        {
            // merge mode, switch to transparent grey rendering
            state.setColour({ 0, 0, 0, 0.05f });

            state.m_blend_src = GL_SRC_ALPHA;
            state.m_blend_dst = GL_ONE_MINUS_SRC_ALPHA;
        }

        // Applicable to both views
        enableViewType(RenderViewType::OrthoView);
        enableViewType(RenderViewType::Camera);
        break;
    }

    default:
        throw std::runtime_error("Cannot construct colour shader type: " + string::to_string(static_cast<int>(_type)));
    }
}

bool ColourShader::supportsVertexColours() const
{
    return !isMergeModeEnabled();
}

void ColourShader::onMergeModeChanged()
{
    // Only certain shaders are reacting to this
    if (_type != ColourShaderType::OrthoviewSolid &&
        _type != ColourShaderType::CameraAndOrthoview &&
        _type != ColourShaderType::CameraAndOrthoViewOutline)
    {
        return;
    }

    // Rebuild the shader, the construct() method will react to the state
    removePasses();
    clearPasses();
    construct();
    insertPasses();
}

std::string ColourShader::ConstructName(ColourShaderType type, const Colour4& colour)
{
    switch (type)
    {
    case ColourShaderType::CameraOutline:
        return fmt::format("<({0:f} {1:f} {2:f})>", colour[0], colour[1], colour[2]);

    case ColourShaderType::CameraSolid:
        return fmt::format("({0:f} {1:f} {2:f})", colour[0], colour[1], colour[2]);

    case ColourShaderType::CameraTranslucent:
        return fmt::format("[{0:f} {1:f} {2:f}]", colour[0], colour[1], colour[2]);

    case ColourShaderType::OrthoviewSolid:
        return fmt::format("<{0:f} {1:f} {2:f}>", colour[0], colour[1], colour[2]);

    case ColourShaderType::CameraAndOrthoview:
        return fmt::format("{{{0:f} {1:f} {2:f}}}", colour[0], colour[1], colour[2]);

    case ColourShaderType::CameraAndOrthoViewOutline:
        return fmt::format("<{{{0:f} {1:f} {2:f}}}>", colour[0], colour[1], colour[2]);
    }

    throw std::runtime_error("Unknown colour shader type: " + string::to_string(static_cast<int>(type)));
}

}
