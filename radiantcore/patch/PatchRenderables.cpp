#include "PatchRenderables.h"

namespace detail
{

inline Vector4 getControlPointVertexColour(std::size_t i, std::size_t width)
{
    static const Vector3& cornerColourVec = GlobalPatchModule().getSettings().getVertexColour(patch::PatchEditVertexType::Corners);
    static const Vector3& insideColourVec = GlobalPatchModule().getSettings().getVertexColour(patch::PatchEditVertexType::Inside);

    return (i % 2 || (i / width) % 2) ? insideColourVec : cornerColourVec;
}

}

RenderablePatchControlPoints::RenderablePatchControlPoints(const IPatch& patch,
    const std::vector<PatchControlInstance>& controlPoints) :
    _patch(patch),
    _controlPoints(controlPoints),
    _needsUpdate(true)
{}

void RenderablePatchControlPoints::updateGeometry()
{
    if (!_needsUpdate) return;

    _needsUpdate = false;

    // Generate the new point vector
    std::vector<render::RenderVertex> vertices;
    std::vector<unsigned int> indices;

    vertices.reserve(_controlPoints.size());
    indices.reserve(_controlPoints.size());

    static const Vector4 SelectedColour(0, 0, 0, 1);
    auto width = _patch.getWidth();

    for (std::size_t i = 0; i < _controlPoints.size(); ++i)
    {
        const auto& ctrl = _controlPoints[i];

        vertices.push_back(render::RenderVertex(ctrl.control.vertex, { 0, 0, 0 }, { 0, 0 },
            ctrl.isSelected() ? SelectedColour : detail::getControlPointVertexColour(i, width)));
        indices.push_back(static_cast<unsigned int>(i));
    }

    updateGeometryWithData(render::GeometryType::Points, vertices, indices);
}
