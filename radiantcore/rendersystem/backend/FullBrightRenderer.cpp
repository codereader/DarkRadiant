#include "FullBrightRenderer.h"

#include "OpenGLShaderPass.h"

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
    // Construct default OpenGL state
    OpenGLState current;
    setupState(current);

    setupViewMatrices(view);

    // Iterate over the sorted mapping between OpenGLStates and their
    // OpenGLShaderPasses (containing the renderable geometry), and render the
    // contents of each bucket. Each pass is passed a reference to the "current"
    // state, which it can change.
    for (const auto& pair : _sortedStates)
    {
        // Render the OpenGLShaderPass
        if (pair.second->empty()) continue;

        if (pair.second->isApplicableTo(_renderViewType))
        {
            pair.second->render(current, globalstate, view.getViewer(), view, time);
        }

        pair.second->clearRenderables();
    }

    cleanupState();

    return std::make_shared<FullBrightRenderResult>(view.getCullStats());
}

}
