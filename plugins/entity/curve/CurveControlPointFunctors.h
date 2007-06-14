#ifndef CURVECONTROLPOINTFUNCTORS_H_
#define CURVECONTROLPOINTFUNCTORS_H_

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
		vector3_snap(point, _snap);
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
		point = _matrix.transform(original).getProjected();
	}
};

class ControlPointAdder :
	public CurveEditInstance::ControlPointFunctor,
	public CurveEditInstance::ControlPointConstFunctor
{
	RenderablePointVector& _points;
	Colour4b _colour;
public:
	ControlPointAdder(RenderablePointVector& points, const Colour4b& colour = colour_vertex) : 
		_points(points),
		_colour(colour)
	{}
	
	// Functor
	void operator()(Vector3& point, const Vector3& original) {
		_points.push_back(PointVertex(Vertex3f(point), _colour));
	}
	
	// ConstFunctor
	void operator()(const Vector3& point, const Vector3& original) {
		_points.push_back(PointVertex(Vertex3f(point), _colour));
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

#endif /*CURVECONTROLPOINTFUNCTORS_H_*/
