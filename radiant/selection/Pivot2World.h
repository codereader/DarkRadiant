#pragma once

#include "Pivot2World.h"
#include "pivot.h"

struct Pivot2World
{
  Matrix4 _worldSpace;
  Matrix4 _viewpointSpace;
  Matrix4 _viewplaneSpace;
  Vector3 _axisScreen;

  void update(const Matrix4& pivot2world, const Matrix4& modelview, const Matrix4& projection, const Matrix4& viewport)
  {
    Pivot2World_worldSpace(_worldSpace, pivot2world, modelview, projection, viewport);
    Pivot2World_viewpointSpace(_viewpointSpace, _axisScreen, pivot2world, modelview, projection, viewport);
    Pivot2World_viewplaneSpace(_viewplaneSpace, pivot2world, modelview, projection, viewport);
  }
};



