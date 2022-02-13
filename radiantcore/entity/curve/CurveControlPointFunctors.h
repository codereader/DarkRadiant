#pragma once

#include "CurveEditInstance.h"

namespace entity {

class ControlPointSnapper :
	public CurveEditInstance::ControlPointFunctor
{
	float _snap;
public:
	ControlPointSnapper(float snap) :
		_snap(snap)
	{}

	void operator()(Vector3& point, const Vector3& original) {
		point.snap(_snap);
	}
};

class ControlPointTransformator :
 	public CurveEditInstance::ControlPointFunctor
{
	const Matrix4& _matrix;
public:
	ControlPointTransformator(const Matrix4& matrix) :
		_matrix(matrix)
	{}

	void operator()(Vector3& point, const Vector3& original) {
		// Take the original (untransformed) point and use this as basis
		point = _matrix.transformPoint(original);
	}
};

class ControlPointBoundsAdder :
	public CurveEditInstance::ControlPointConstFunctor
{
	AABB& _bounds;
public:
	ControlPointBoundsAdder(AABB& bounds) :
		_bounds(bounds)
	{}

	void operator()(const Vector3& point, const Vector3& original) {
		_bounds.includePoint(point);
	}
};

} // namespace entity
