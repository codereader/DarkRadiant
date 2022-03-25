#pragma once

#include "ivolumetest.h"
#include "math/Matrix4.h"

namespace render
{

/**
 * greebo: This is a minimal VolumeTest implementation which
 * returns true in all test cases. All matrices are identity
 * unless they're set to something different.
 */
class NopVolumeTest :
	public VolumeTest
{
private:
	Matrix4 _viewPort;
	Matrix4 _projection;
	Matrix4 _modelView;
	Matrix4 _viewProjection;

public:
	NopVolumeTest() :
		_viewPort(Matrix4::getIdentity()),
		_projection(Matrix4::getIdentity()),
		_modelView(Matrix4::getIdentity()),
		_viewProjection(Matrix4::getIdentity())
	{}

	bool TestPoint(const Vector3& point) const
	{ 
		return true; 
	}

	bool TestLine(const Segment& segment) const
	{ 
		return true;
	}

	bool TestPlane(const Plane3& plane) const
	{ 
		return true;
	}

	bool TestPlane(const Plane3& plane, const Matrix4& localToWorld) const 
	{ 
		return true;
	}

	VolumeIntersectionValue TestAABB(const AABB& aabb) const
	{ 
		return VOLUME_INSIDE; 
	}

	VolumeIntersectionValue TestAABB(const AABB& aabb, const Matrix4& localToWorld) const
	{
		return VOLUME_INSIDE;
	}

	virtual bool fill() const
	{ 
		return true;
	}

	virtual const Matrix4& GetViewProjection() const
	{
		return _viewProjection;
	}

	virtual const Matrix4& GetViewport() const
	{
		return _viewPort;
	}

	virtual const Matrix4& GetProjection() const
	{
		return _projection;
	}

	virtual const Matrix4& GetModelview() const
	{
		return _modelView;
	}

	void setViewPort(const Matrix4& matrix)
	{
		_viewPort = matrix;
	}

	void setProjection(const Matrix4& matrix)
	{
		_projection = matrix;
	}

	void setModelView(const Matrix4& matrix)
	{
		_modelView = matrix;
	}

    void setViewProjection(const Matrix4& matrix)
	{
        _viewProjection = matrix;
	}
};

} // namespace render
