#pragma once

#include "irenderview.h"

#include "math/Frustum.h"
#include "math/ViewProjection.h"
#include "math/Viewer.h"

#if 0
#define DEBUG_CULLING
#endif

#ifdef DEBUG_CULLING
#include <fmt/format.h>

#define INC_COUNTER(x) (++(const_cast<View*>(this)->x))
#else
#define INC_COUNTER(x)
#endif

namespace render
{

/// \brief View-volume culling and transformations.
class View: public IRenderView
{
private:
	/// modelview matrix
	Matrix4 _modelview;

	/// projection matrix
	Matrix4 _projection;

	/// device-to-screen transform
	Matrix4 _viewport;

	Matrix4 _scissor;

	/// combined modelview and projection matrix
	ViewProjection _viewproj;

	/// camera position in world space
	Viewer _viewer;

	/// view frustum in world space
	Frustum _frustum;

	bool _fill;

#ifdef DEBUG_CULLING
	int _count_planes;
	int _count_oriented_planes;
	int _count_bboxs;
	int _count_oriented_bboxs;
#endif

public:
	View(bool fill = false) :
		_modelview(Matrix4::getIdentity()),
		_projection(Matrix4::getIdentity()),
		_scissor(Matrix4::getIdentity()),
		_fill(fill)
	{}

	View(const View& other) = default;

	View(const VolumeTest& other) :
		_modelview(other.GetModelview()),
		_projection(other.GetProjection()),
		_viewport(other.GetViewport()),
		_scissor(Matrix4::getIdentity()),
		_fill(other.fill())
	{
		// calculate _viewproj, _viewer and _frustum
		construct();
	}

	void construct(const Matrix4& projection, const Matrix4& modelview, std::size_t width, std::size_t height) override
	{
		// modelview
		_modelview = modelview;

		// projection
		_projection = projection;

		// viewport
		_viewport = Matrix4::getIdentity();
		_viewport[0] = float(width / 2);
		_viewport[5] = float(height / 2);
		if (fabs(_projection[11]) > 0.0000001)
			_viewport[10] = _projection[0] * _viewport[0];
		else
			_viewport[10] = 1 / _projection[10];

		construct();
	}

	void EnableScissor(double min_x, double max_x, double min_y, double max_y)
	{
		_scissor = Matrix4::getIdentity();
		_scissor[0] = (max_x - min_x) * 0.5;
		_scissor[5] = (max_y - min_y) * 0.5;
		_scissor[12] = (min_x + max_x) * 0.5;
		_scissor[13] = (min_y + max_y) * 0.5;
		_scissor.invertFull();

		construct();
	}

	void DisableScissor()
	{
		_scissor = Matrix4::getIdentity();

		construct();
	}

	bool TestPoint(const Vector3& point) const override
	{
		return _viewproj.testPoint(point);
	}

	bool TestLine(const Segment& segment) const override
	{
		return _frustum.testLine(segment);
	}

	bool TestPlane(const Plane3& plane) const override
	{
		INC_COUNTER(_count_planes);
		return _viewer.testPlane(plane);
	}

	bool TestPlane(const Plane3& plane, const Matrix4& localToWorld) const override
	{
		INC_COUNTER(_count_oriented_planes);
		return _viewer.testPlane(plane, localToWorld);
	}

    VolumeIntersectionValue TestAABB(const AABB& aabb) const override
	{
		INC_COUNTER(_count_bboxs);
		return _frustum.testIntersection(aabb);
	}

	VolumeIntersectionValue TestAABB(const AABB& aabb, const Matrix4& localToWorld) const override
	{
		INC_COUNTER(_count_oriented_bboxs);
		return _frustum.testIntersection(aabb, localToWorld);
	}

	const Matrix4& GetViewProjection() const override
	{
		return _viewproj;
	}

	const Matrix4& GetViewport() const override
	{
		return _viewport;
	}

	const Matrix4& GetModelview() const override
	{
		return _modelview;
	}

	const Matrix4& GetProjection() const override
	{
		return _projection;
	}

	bool fill() const override
	{
		return _fill;
	}

	Vector3 getViewer() const override
	{
		return _viewer.getVector3();
	}

    const Frustum& getFrustum() const override
	{
		return _frustum;
	}

	std::string getCullStats() const override
	{
		std::string stats;

#if defined(DEBUG_CULLING)
		stats = fmt::format("planes {0:d} + {1:d} | bboxs {2:d} + {3:d}",
			_count_planes, _count_oriented_planes,
			_count_bboxs, _count_oriented_bboxs);
#endif

		return stats;
	}

	void resetCullStats()
	{
#if defined(DEBUG_CULLING)
		_count_planes = 0;
		_count_oriented_planes = 0;
		_count_bboxs = 0;
		_count_oriented_bboxs = 0;
#endif
	}

private:
	void construct()
	{
		_viewproj = _scissor.getMultipliedBy(_projection).getMultipliedBy(_modelview);

		_frustum = Frustum::createFromViewproj(_viewproj);
		_viewer = Viewer::createFromViewProjection(_viewproj);
	}
};

} // namespace
