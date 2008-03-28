#ifndef BRUSH_CSG_H_
#define BRUSH_CSG_H_

#include "iclipper.h"
#include "math/Plane3.h"

// Contains the routines for brush subtract, merge and hollow

class BrushNode;
typedef boost::shared_ptr<BrushNode> BrushNodePtr;

class Plane3;
class Brush;

namespace brush {
namespace algorithm {

/** 
 * greebo: Hollows the given brush. Note that this selects the 
 *         resulting brushes and removes the source brush from the scene.
 *
 * @makeRoom: Set this to TRUE to move the wall brushes outside a bit 
 *            (see makeRoomForSelectedBrushes() routine).
 */
void hollowBrush(const BrushNodePtr& sourceBrush, bool makeRoom);

/**
 * greebo: Hollows all currently selected brushes.
 */
void hollowSelectedBrushes();

/** greebo: This tries to move the newly created brushes towards the outside
 * 			so that the corners don't overlap (works only for rectangular prisms). 
 */
void makeRoomForSelectedBrushes();

/**
 * greebo: Subtracts the brushes from all surrounding unselected brushes. 
 */
void subtractBrushesFromUnselected();

/**
 * greebo: Attempts to merge the selected brushes.
 */
void mergeSelectedBrushes();

/**
 * greebo: Sets the "clip plane" of the selected brushes in the scene.
 */
void setBrushClipPlane(const Plane3& plane);

// Classifies the given brush (needed for clipping/csg)
BrushSplitType Brush_classifyPlane(const Brush& brush, const Plane3& plane);

/**
 * greebo: Splits the selected brushes by the given plane.
 */
void splitBrushesByPlane(const Vector3 planePoints[3], const std::string& shader, EBrushSplit split);

} // namespace algorihtm
} // namespace brush

#endif /* BRUSH_CSG_H_ */
