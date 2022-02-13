#include "BuiltInShader.h"

#include "string/convert.h"
#include "icolourscheme.h"

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

        if (canUseLightingMode())
        {
            appendDepthFillPass();

            // Append a blend layer
            OpenGLState& state = appendDefaultPass();
            state.setRenderFlag(RENDER_FILL);
            state.setRenderFlag(RENDER_BLEND);
            state.setRenderFlag(RENDER_DEPTHTEST);
            state.setDepthFunc(GL_LEQUAL);

            // Remember the stage for later evaluation of shader expressions
            state.stage0 = nullptr;

            // Set the texture
            pass.texture0 = editorTex ? editorTex->getGLTexNum() : 0;

            // Set the blend ADD function
            state.m_blend_src = GL_ONE;
            state.m_blend_dst = GL_ONE;

            state.setRenderFlag(RENDER_DEPTHWRITE);
            state.setRenderFlag(RENDER_TEXTURE_2D);

            // Colour modulation
            state.setColour(Colour4::WHITE());

            // Sort position
            state.setSortPosition(OpenGLState::SORT_FULLBRIGHT);
        }
        else
        {
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
        }

        enableViewType(RenderViewType::Camera);
        break;
    }

    case BuiltInShaderType::BrushClipPlane:
    {
        pass.setColour(GlobalColourSchemeManager().getColour("clipper"));
        pass.setRenderFlags(RENDER_CULLFACE
            | RENDER_DEPTHWRITE
            | RENDER_FILL
            | RENDER_POLYGONSTIPPLE);
        pass.setSortPosition(OpenGLState::SORT_OVERLAY_FIRST);

        enableViewType(RenderViewType::Camera);
        enableViewType(RenderViewType::OrthoView);
        break;
    }
    
    case BuiltInShaderType::WireframeOverlay:
    {
        pass.setRenderFlags(RENDER_DEPTHWRITE
            | RENDER_DEPTHTEST
            | RENDER_OVERRIDE
            | RENDER_VERTEX_COLOUR);
        pass.setSortPosition(OpenGLState::SORT_GUI1);
        pass.setDepthFunc(GL_LEQUAL);

        OpenGLState& hiddenLine = appendDefaultPass();
        hiddenLine.setName(getName() + "_Hidden");
        hiddenLine.setRenderFlags(RENDER_DEPTHWRITE
            | RENDER_DEPTHTEST
            | RENDER_OVERRIDE
            | RENDER_LINESTIPPLE
            | RENDER_VERTEX_COLOUR);
        hiddenLine.setSortPosition(OpenGLState::SORT_GUI0);
        hiddenLine.setDepthFunc(GL_GREATER);

        enableViewType(RenderViewType::Camera);
        enableViewType(RenderViewType::OrthoView);
        break;
    }

    case BuiltInShaderType::PointTraceLines:
    {
        pass.setColour(1, 0, 0, 1);
        pass.setRenderFlags(RENDER_DEPTHTEST | RENDER_DEPTHWRITE);
        pass.setSortPosition(OpenGLState::SORT_FULLBRIGHT);
        pass.m_linewidth = 4;

        enableViewType(RenderViewType::Camera);
        enableViewType(RenderViewType::OrthoView);
        break;
    }

    case BuiltInShaderType::ColouredPolygonOverlay:
    {
        // This is the shader drawing a coloured overlay
        // over faces/polys. Its colour is configurable,
        // and it has depth test activated.
        pass.setRenderFlag(RENDER_FILL);
        pass.setRenderFlag(RENDER_DEPTHTEST);
        pass.setRenderFlag(RENDER_CULLFACE);
        pass.setRenderFlag(RENDER_BLEND);

        pass.setColour({ GlobalColourSchemeManager().getColour("selected_brush_camera"), 0.3f });
        pass.setSortPosition(OpenGLState::SORT_HIGHLIGHT);
        pass.polygonOffset = 0.5f;
        pass.setDepthFunc(GL_LEQUAL);

        enableViewType(RenderViewType::Camera);
        break;
    }

    case BuiltInShaderType::HighlightedPolygonOutline:
    {
        // This is the shader drawing a solid line to outline
        // a selected item. The first pass has its depth test
        // activated using GL_LESS, whereas the second pass
        // draws the hidden lines in stippled appearance
        // with its depth test using GL_GREATER.
        pass.setRenderFlags(RENDER_OFFSETLINE | RENDER_DEPTHTEST);
        pass.setSortPosition(OpenGLState::SORT_OVERLAY_LAST);

        // Second pass for hidden lines
        OpenGLState& hiddenLine = appendDefaultPass();
        hiddenLine.setColour(0.75f, 0.75f, 0.75f, 1);
        hiddenLine.setRenderFlags(RENDER_CULLFACE
            | RENDER_DEPTHTEST
            | RENDER_OFFSETLINE
            | RENDER_LINESTIPPLE);
        hiddenLine.setSortPosition(OpenGLState::SORT_OVERLAY_FIRST);
        hiddenLine.setDepthFunc(GL_GREATER);
        hiddenLine.m_linestipple_factor = 2;

        enableViewType(RenderViewType::Camera);
        break;
    }

    case BuiltInShaderType::WireframeSelectionOverlay:
    {
        constructWireframeSelectionOverlay(pass, "selected_brush");
        break;
    }
    
    case BuiltInShaderType::WireframeSelectionOverlayOfGroups:
    {
        constructWireframeSelectionOverlay(pass, "selected_group_items");
        break;
    }

    case BuiltInShaderType::Point:
    {
        constructPointShader(pass, 4, OpenGLState::SORT_POINT_FIRST);
        break;
    }

    case BuiltInShaderType::BigPoint:
    {
        constructPointShader(pass, 6, OpenGLState::SORT_POINT_FIRST);
        break;
    }

    case BuiltInShaderType::Pivot:
    {
        pass.setRenderFlags(RENDER_DEPTHTEST | RENDER_DEPTHWRITE);
        pass.setSortPosition(OpenGLState::SORT_GUI0);
        pass.m_linewidth = 2;
        pass.setDepthFunc(GL_LEQUAL);

        OpenGLState& hiddenLine = appendDefaultPass();
        hiddenLine.setName(getName() + "_Hidden");
        hiddenLine.setRenderFlags(RENDER_DEPTHTEST | RENDER_LINESTIPPLE);
        hiddenLine.setSortPosition(OpenGLState::SORT_GUI0);
        hiddenLine.m_linewidth = 2;
        hiddenLine.setDepthFunc(GL_GREATER);

        enableViewType(RenderViewType::Camera);
        enableViewType(RenderViewType::OrthoView);
        break;
    }

    case BuiltInShaderType::PatchLattice:
    {
        pass.setColour(1, 0.5, 0, 1);
        pass.setRenderFlag(RENDER_DEPTHWRITE);
        pass.setSortPosition(OpenGLState::SORT_POINT_FIRST);

        enableViewType(RenderViewType::Camera);
        enableViewType(RenderViewType::OrthoView);
        break;
    }

    case BuiltInShaderType::CameraMergeActionOverlayAdd:
    {
        // render additions over removals
        constructCameraMergeActionOverlay(pass, { 0, 0.9f, 0, 0.5f },
            OpenGLState::SORT_OVERLAY_THIRD, OpenGLState::SORT_OVERLAY_LAST);
        break;
    }

    case BuiltInShaderType::CameraMergeActionOverlayRemove:
    {
        constructCameraMergeActionOverlay(pass, { 0.6f, 0.1f, 0, 0.5f },
            OpenGLState::SORT_OVERLAY_FIRST, OpenGLState::SORT_OVERLAY_ONE_BEFORE_LAST);
        break;
    }

    case BuiltInShaderType::CameraMergeActionOverlayChange:
    {
        constructCameraMergeActionOverlay(pass, { 0, 0.4f, 0.9f, 0.5f },
            OpenGLState::SORT_OVERLAY_SECOND, OpenGLState::SORT_OVERLAY_LAST);
        break;
    }

    case BuiltInShaderType::CameraMergeActionOverlayConflict:
    {
        constructCameraMergeActionOverlay(pass, { 0.9f, 0.5f, 0.0f, 0.5f },
            OpenGLState::SORT_OVERLAY_ONE_BEFORE_LAST, OpenGLState::SORT_OVERLAY_LAST);
        break;
    }

    case BuiltInShaderType::OrthoMergeActionOverlayAdd:
    {
        // render additions over removals
        constructOrthoMergeActionOverlay(pass, { 0, 0.5f, 0, 0.5f }, OpenGLState::SORT_OVERLAY_THIRD);
        break;
    }

    case BuiltInShaderType::OrthoMergeActionOverlayRemove:
    {
        constructOrthoMergeActionOverlay(pass, { 0.6f, 0.1f, 0, 0.5f }, OpenGLState::SORT_OVERLAY_FIRST);
        break;
    }

    case BuiltInShaderType::OrthoMergeActionOverlayChange:
    {
        constructOrthoMergeActionOverlay(pass, { 0, 0.4f, 0.9f, 0.5f }, OpenGLState::SORT_OVERLAY_SECOND);
        break;
    }

    case BuiltInShaderType::OrthoMergeActionOverlayConflict:
    {
        constructOrthoMergeActionOverlay(pass, { 0.9f, 0.5f, 0.0f, 0.5f }, OpenGLState::SORT_OVERLAY_ONE_BEFORE_LAST);
        break;
    }

    default:
        throw std::runtime_error("Cannot construct this shader: " + getName());
    }
}

void BuiltInShader::constructOrthoMergeActionOverlay(OpenGLState& pass, const Colour4& colour,
    OpenGLState::SortPosition sortPosition)
{
    pass.setColour(colour);
    pass.setSortPosition(OpenGLState::SORT_OVERLAY_FIRST);
    pass.m_linewidth = 2;

    enableViewType(RenderViewType::OrthoView);
}

void BuiltInShader::constructCameraMergeActionOverlay(OpenGLState& pass, const Colour4& colour,
    OpenGLState::SortPosition sortPosition, OpenGLState::SortPosition lineSortPosition)
{
    // This is a pass drawing a coloured overlay
    // over faces/polys. Its colour is configurable,
    // and it has depth test activated.
    pass.setRenderFlag(RENDER_FILL);
    pass.setRenderFlag(RENDER_DEPTHTEST);
    pass.setRenderFlag(RENDER_CULLFACE);
    pass.setRenderFlag(RENDER_BLEND);

    pass.setColour(colour);
    pass.setSortPosition(sortPosition);
    pass.polygonOffset = 0.5f;
    pass.setDepthFunc(GL_LEQUAL);

    // This is the outline pass
    auto& linesOverlay = appendDefaultPass();
    auto lineOverlayColour = colour;
    lineOverlayColour[3] = 0.78f;

    linesOverlay.setColour(lineOverlayColour);
    linesOverlay.setRenderFlags(RENDER_OFFSETLINE | RENDER_DEPTHTEST | RENDER_BLEND);
    linesOverlay.setSortPosition(lineSortPosition);

    enableViewType(RenderViewType::Camera);
}

void BuiltInShader::constructPointShader(OpenGLState& pass, float pointSize, OpenGLState::SortPosition sort)
{
    pass.setRenderFlag(RENDER_POINT_COLOUR);
    pass.setRenderFlag(RENDER_DEPTHWRITE);

    pass.setSortPosition(sort);
    pass.m_pointsize = pointSize;

    enableViewType(RenderViewType::Camera);
    enableViewType(RenderViewType::OrthoView);
}

void BuiltInShader::constructWireframeSelectionOverlay(OpenGLState& pass, const std::string& schemeColourKey)
{
    auto colorSelBrushes = GlobalColourSchemeManager().getColour(schemeColourKey);
    pass.setColour({ colorSelBrushes, 1 });
    pass.setRenderFlag(RENDER_LINESTIPPLE);
    pass.setSortPosition(OpenGLState::SORT_HIGHLIGHT);
    pass.m_linewidth = 2;
    pass.m_linestipple_factor = 3;

    enableViewType(RenderViewType::OrthoView);
}

std::string BuiltInShader::GetNameForType(BuiltInShaderType type)
{
    return "$BUILT_IN_SHADER[" + string::to_string(static_cast<std::size_t>(type)) + "]";
}

}
