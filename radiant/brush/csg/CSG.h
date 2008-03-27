#ifndef BRUSH_CSG_H_
#define BRUSH_CSG_H_

#include <boost/shared_ptr.hpp>

// Contains the routines for brush subtract, merge and hollow

class BrushNode;
typedef boost::shared_ptr<BrushNode> BrushNodePtr;

namespace algorithm {
namespace csg {

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

} // namespace csg
} // namespace algorihtm

#endif /* BRUSH_CSG_H_ */
