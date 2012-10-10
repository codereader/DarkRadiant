#pragma once

#include "ivolumetest.h"

#include "math/Frustum.h"
#include "math/ViewProjection.h"
#include "math/Viewer.h"

namespace render
{

/// \brief View-volume culling and transformations.
class View : 
	public VolumeTest
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
	
public:
	View(bool fill = false);

	void Construct(const Matrix4& projection, const Matrix4& modelview, std::size_t width, std::size_t height);

	void EnableScissor(float min_x, float max_x, float min_y, float max_y);
	void DisableScissor();

	bool TestPoint(const Vector3& point) const;
	bool TestLine(const Segment& segment) const;
	bool TestPlane(const Plane3& plane) const;
	bool TestPlane(const Plane3& plane, const Matrix4& localToWorld) const;

    VolumeIntersectionValue TestAABB(const AABB& aabb) const;
	VolumeIntersectionValue TestAABB(const AABB& aabb, const Matrix4& localToWorld) const;

	const Matrix4& GetViewMatrix() const;
	const Matrix4& GetViewport() const;
	const Matrix4& GetModelview() const;
	const Matrix4& GetProjection() const;

	bool fill() const;
	
	const Vector3& getViewer() const;
	
	static const std::string& getCullStats();
	static void resetCullStats();

private:
	void construct();
};

} // namespace
