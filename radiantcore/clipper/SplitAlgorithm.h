#pragma once

#include "iclipper.h"
#include "math/Vector3.h"

class Plane3;

namespace algorithm
{

/**
 * greebo: Sets the "clip plane" of the selected brushes in the scene.
 */
void setBrushClipPlane(const Plane3& plane);

/**
 * greebo: Splits the selected brushes by the given plane.
 */
void splitBrushesByPlane(const Vector3 planePoints[3], EBrushSplit split);

}
