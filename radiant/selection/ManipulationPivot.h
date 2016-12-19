#pragma once

#include "math/Matrix4.h"

namespace selection
{

/**
 * Represents the anchor point for manipulation operations
 * in the scene. Usually this is defined by the origin of the
 * current selection AABB.
 */
class ManipulationPivot
{
private:
	Matrix4 _pivot2World;

public:
	const Matrix4& getMatrix4() const
	{
		return _pivot2World;
	}

	void setFromMatrix(const Matrix4& newPivot2World)
	{
		_pivot2World = newPivot2World;
	}

	void translate(const Vector3& translation)
	{
		_pivot2World.translateBy(translation);
	}
};

}
