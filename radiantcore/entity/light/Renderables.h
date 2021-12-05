#pragma once

#include "math/Vector3.h"
#include "math/Vector4.h"
#include "math/Line.h"
#include "math/Frustum.h"
#include "entitylib.h"
#include "igl.h"
#include "render/RenderableGeometry.h"
#include "../OriginKey.h"

void light_draw_box_lines(const Vector3& origin, const Vector3 points[8]);

namespace entity {

class RenderLightRadiiBox : public OpenGLRenderable {
	const Vector3& m_origin;
public:
	mutable Vector3 m_points[8];
	static ShaderPtr m_state;

	RenderLightRadiiBox(const Vector3& origin) : m_origin(origin) {}

	void render(const RenderInfo& info) const;
}; // class RenderLightRadiiBox

class RenderLightProjection : public OpenGLRenderable {
	const Vector3& _origin;
	const Vector3& _start;
	const Frustum& _frustum;
public:
	RenderLightProjection(const Vector3& origin, const Vector3& start, const Frustum& frustum);

	// greebo: Renders the light cone of a projected light (may also be a frustum, when light_start / light_end are set)
	void render(const RenderInfo& info) const;
}; // class RenderLightProjection

// The small diamond representing at the light's origin
class RenderableLightOctagon :
    public render::RenderableGeometry
{
private:
    const Vector3& _origin;
    bool _needsUpdate;

public:
    RenderableLightOctagon(const Vector3& origin) :
        _origin(origin),
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

        // Generate the indexed vertex data
        static Vector3 Extents(8, 8, 8);

        // Calculate the light vertices of this bounding box and store them into <points>
        Vector3 max(_origin + Extents);
        Vector3 min(_origin - Extents);
        Vector3 mid(_origin);

        // top, bottom, tleft, tright, bright, bleft
        std::vector<ArbitraryMeshVertex> vertices
        {
            ArbitraryMeshVertex({ mid[0], mid[1], max[2] }, {1,0,0}, {0,0}),
            ArbitraryMeshVertex({ mid[0], mid[1], min[2] }, {1,0,0}, {0,0}),
            ArbitraryMeshVertex({ min[0], max[1], mid[2] }, {1,0,0}, {0,0}),
            ArbitraryMeshVertex({ max[0], max[1], mid[2] }, {1,0,0}, {0,0}),
            ArbitraryMeshVertex({ max[0], min[1], mid[2] }, {1,0,0}, {0,0}),
            ArbitraryMeshVertex({ min[0], min[1], mid[2] }, {1,0,0}, {0,0}),
        };

        // Indices are always the same, therefore constant
        static const std::vector<unsigned int> Indices
        {
            0, 2, 3,
            0, 3, 4,
            0, 4, 5,
            0, 5, 2,
            1, 2, 5,
            1, 5, 4,
            1, 4, 3,
            1, 3, 2
        };

        RenderableGeometry::updateGeometry(render::GeometryType::Triangles, vertices, Indices);
    }
};

} // namespace entity
