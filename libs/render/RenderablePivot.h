#pragma once

#include "render/RenderableGeometry.h"

#include "math/Vector3.h"
#include "math/Vector4.h"

namespace render
{

class RenderablePivot :
	public RenderableGeometry
{
private:
	const Vector3& _pivot;
    bool _needsUpdate;

public:
    // Pass a reference to the pivot is in world coordinates
    RenderablePivot(const Vector3& pivot) :
        _pivot(pivot),
        _needsUpdate(true)
    {}

    void queueUpdate()
    {
        _needsUpdate = true;
    }

protected:
    void updateGeometry() override
    {
        if (!_needsUpdate) return;

        _needsUpdate = false;

        static const Vector4 ColourX{ 255, 0, 0, 255 };
        static const Vector4 ColourY{ 0, 255, 0, 255 };
        static const Vector4 ColourZ{ 0, 0, 255, 255 };

        std::vector<ArbitraryMeshVertex> vertices;

        vertices.push_back(ArbitraryMeshVertex(_pivot, { 0, 0, 0 }, { 0, 0 }, ColourX));
        vertices.push_back(ArbitraryMeshVertex(_pivot + Vector3(16, 0, 0), { 0, 0, 0 }, { 0, 0 }, ColourX));

        vertices.push_back(ArbitraryMeshVertex(_pivot, { 0, 0, 0 }, { 0, 0 }, ColourY));
        vertices.push_back(ArbitraryMeshVertex(_pivot + Vector3(0, 16, 0), { 0, 0, 0 }, { 0, 0 }, ColourY));

        vertices.push_back(ArbitraryMeshVertex(_pivot, { 0, 0, 0 }, { 0, 0 }, ColourZ));
        vertices.push_back(ArbitraryMeshVertex(_pivot + Vector3(0, 0, 16), { 0, 0, 0 }, { 0, 0 }, ColourZ));

        static std::vector<unsigned int> Indices =
        {
            0, 1,
            2, 3,
            4, 5
        };

        RenderableGeometry::updateGeometry(GeometryType::Lines, vertices, Indices);
    }
};

}
