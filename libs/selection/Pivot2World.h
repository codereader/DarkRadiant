#pragma once

#include "pivot.h"

namespace selection
{

class Pivot2World
{
public:
	Matrix4 _worldSpace;
	Matrix4 _viewpointSpace;
	Matrix4 _viewplaneSpace;
	Vector3 _axisScreen;

	void update(const Matrix4& pivot2world, const Matrix4& modelview, const Matrix4& projection, const Matrix4& viewport)
	{
		calculcateWorldSpace(pivot2world, modelview, projection, viewport);
		calculateViewpointSpace(pivot2world, modelview, projection, viewport);
		calculateViewplaneSpace(pivot2world, modelview, projection, viewport);
	}

private:
	void calculcateWorldSpace(const Matrix4& pivot2world, const Matrix4& modelview, const Matrix4& projection, const Matrix4& viewport)
	{
		_worldSpace = pivot2world;

		Matrix4 pivot2screen = constructObject2Screen(pivot2world, modelview, projection, viewport);

		Matrix4 scale = getInverseScale(pivot2screen);

		_worldSpace.multiplyBy(scale);
		scale = getPerspectiveScale(pivot2screen);
		_worldSpace.multiplyBy(scale);
	}

	void calculateViewpointSpace(const Matrix4& pivot2world, const Matrix4& modelview, const Matrix4& projection, const Matrix4& viewport)
	{
		_viewpointSpace = pivot2world;

		Matrix4 pivot2screen = constructObject2Screen(pivot2world, modelview, projection, viewport);

		Matrix4 scale = getInverseScale(pivot2screen);

		_viewpointSpace.multiplyBy(scale);

		billboard_viewpointOriented(scale, pivot2screen);
		_axisScreen = scale.z().getVector3();
		_viewpointSpace.multiplyBy(scale);

		scale = getPerspectiveScale(pivot2screen);
		_viewpointSpace.multiplyBy(scale);
	}

	void calculateViewplaneSpace(const Matrix4& pivot2world, const Matrix4& modelview, const Matrix4& projection, const Matrix4& viewport)
	{
		_viewplaneSpace = pivot2world;

		Matrix4 pivot2screen = constructObject2Screen(pivot2world, modelview, projection, viewport);

		Matrix4 scale = getInverseScale(pivot2screen);
		
		_viewplaneSpace.multiplyBy(scale);

		billboard_viewplaneOriented(scale, pivot2screen);
		_viewplaneSpace.multiplyBy(scale);

		scale = getPerspectiveScale(pivot2screen);
		_viewplaneSpace.multiplyBy(scale);
	}
};

}
