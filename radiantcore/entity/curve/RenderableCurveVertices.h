#pragma once

#include "Curve.h"
#include "CurveEditInstance.h"
#include "render/RenderableGeometry.h"

namespace entity
{

class RenderableCurveVertices :
    public render::RenderableGeometry
{
private:
    const Curve& _curve;
    const CurveEditInstance& _instance;
    bool _updateNeeded;

public:
    RenderableCurveVertices(const Curve& curve, const CurveEditInstance& instance) :
        _curve(curve),
        _instance(instance),
        _updateNeeded(true)
    {}

    void queueUpdate()
    {
        _updateNeeded = true;
    }

protected:
    void updateGeometry() override
    {
        if (!_updateNeeded) return;

        _updateNeeded = false;

        if (_curve.isEmpty())
        {
            clear();
            return;
        }

        std::vector<render::RenderVertex> vertices;
        std::vector<unsigned int> indices;

        static const Vector4 SelectedColour(0, 0, 0, 1);
        static const Vector4 DeselectedColour(0, 1, 0, 1);

        auto i = 0;

        _instance.forEachControlPoint([&](const Vector3& position, bool isSelected)
        {
            vertices.push_back(render::RenderVertex(position, { 0, 0, 0 }, { 0, 0 },
                isSelected ? SelectedColour : DeselectedColour));
            indices.push_back(static_cast<unsigned int>(i++));
        });

        updateGeometryWithData(render::GeometryType::Points, vertices, indices);
    }
};

}
