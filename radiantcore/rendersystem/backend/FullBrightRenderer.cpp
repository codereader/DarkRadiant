#include "FullBrightRenderer.h"

#include "OpenGLShaderPass.h"
#include "OpenGLShader.h"

namespace render
{

namespace
{

class FullBrightRenderResult : 
    public IRenderResult
{
private:
    std::string _statistics;

public:
    FullBrightRenderResult(const std::string& statistics) :
        _statistics(statistics)
    {}

    std::string toString() override
    {
        return _statistics;
    }
};

}

IRenderResult::Ptr FullBrightRenderer::render(RenderStateFlags globalstate, const IRenderView& view, std::size_t time)
{
    // Make sure all the geometry is ready for rendering
    for (const auto& [_, pass] : _sortedStates)
    {
        pass->getShader().prepareForRendering();
    }

    // Make sure all the data is uploaded
    _geometryStore.syncToBufferObjects();

    // Construct default OpenGL state
    OpenGLState current;
    setupState(current);

    setupViewMatrices(view);

    // Bind the vertex and index buffer object before drawing geometry
    auto [vertexBuffer, indexBuffer] = _geometryStore.getBufferObjects();

    vertexBuffer->bind();
    indexBuffer->bind();

    // Set the attribute pointers
    ObjectRenderer::InitAttributePointers();

    // Iterate over the sorted mapping between OpenGLStates and their
    // OpenGLShaderPasses (containing the renderable geometry), and render the
    // contents of each bucket. Each pass is passed a reference to the "current"
    // state, which it can change.
    for (const auto& [_, pass] : _sortedStates)
    {
        // Render the OpenGLShaderPass
        if (pass->empty() || pass->hasRenderables()) continue;

        if (pass->getShader().isVisible() && pass->isApplicableTo(_renderViewType))
        {
            // Apply our state to the current state object
            pass->evaluateStagesAndApplyState(current, globalstate, time, nullptr);
            pass->submitSurfaces(view);
        }
    }

    // Unbind the geometry buffer and draw the rest of the renderables
    vertexBuffer->unbind();
    indexBuffer->unbind();

    // Run all the passes with OpenGLRenderables
    for (const auto& [_, pass] : _sortedStates)
    {
        // Render the OpenGLShaderPass
        if (pass->empty() || !pass->hasRenderables()) continue;

        if (pass->getShader().isVisible() && pass->isApplicableTo(_renderViewType))
        {
            // Apply our state to the current state object
            pass->evaluateStagesAndApplyState(current, globalstate, time, nullptr);
            pass->submitRenderables(current);
        }

        pass->clearRenderables();
    }

    cleanupState();

    return std::make_shared<FullBrightRenderResult>(view.getCullStats());
}

}
