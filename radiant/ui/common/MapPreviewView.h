#ifndef _MAP_PREVIEW_VIEW_H_
#define _MAP_PREVIEW_VIEW_H_

#include "cullable.h"
#include "math/matrix.h"

namespace ui {

/**
 * greebo: This is more or less a stub class implementing the
 * needed routines of the VolumeTest abstract base class.
 *
 * Most routines just return TRUE or positive, so this class
 * won't do any culling.
 */
class MapPreviewView : 
	public VolumeTest
{
public:
	Matrix4 modelView;
	Matrix4 projection;
	Matrix4 viewport;

	MapPreviewView() :
		modelView(Matrix4::getIdentity()),
		projection(Matrix4::getIdentity()),
		viewport(Matrix4::getIdentity())
	{}

	/// \brief Returns true if \p point intersects volume.
	bool TestPoint(const Vector3& point) const {
		return true;
	}

	/// \brief Returns true if \p segment intersects volume.
	bool TestLine(const Segment& segment) const {
		return true;
	}

	/// \brief Returns true if \p plane faces towards volume.
	bool TestPlane(const Plane3& plane) const {
		return true;
	}

	/// \brief Returns true if \p plane transformed by \p localToWorld faces the viewer.
	bool TestPlane(const Plane3& plane, const Matrix4& localToWorld) const {
		return true;
	}

	/// \brief Returns the intersection of \p aabb and volume.
	VolumeIntersectionValue TestAABB(const AABB& aabb) const {
		return VOLUME_INSIDE;
	}

	/// \brief Returns the intersection of \p aabb transformed by \p localToWorld and volume.
	VolumeIntersectionValue TestAABB(const AABB& aabb, const Matrix4& localToWorld) const {
		return VOLUME_INSIDE;
	}

	bool fill() const {
		return true;
	}

	const Matrix4& GetViewport() const {
		return viewport;
	}

	const Matrix4& GetProjection() const {
		return projection;
	}

	const Matrix4& GetModelview() const {
		return modelView;
	};
};

} // namespace ui

#endif /* _MAP_PREVIEW_VIEW_H_ */
