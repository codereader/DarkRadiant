#pragma once

#include "iclipper.h"
#include "icommandsystem.h"
#include "math/Plane3.h"

// Contains the routines for brush subtract, merge and hollow

class BrushNode;
typedef std::shared_ptr<BrushNode> BrushNodePtr;

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
void hollowSelectedBrushes(const cmd::ArgumentList& args);

/** greebo: This tries to move the newly created brushes towards the outside
 * 			so that the corners don't overlap (works only for rectangular prisms).
 */
void makeRoomForSelectedBrushes(const cmd::ArgumentList& args);

/**
 * greebo: Subtracts the brushes from all surrounding unselected brushes.
 */
void subtractBrushesFromUnselected(const cmd::ArgumentList& args);

/**
 * greebo: Attempts to merge the selected brushes.
 */
void mergeSelectedBrushes(const cmd::ArgumentList& args);

/**
 * Connect the various events to the functions in this namespace
 */
void registerCommands();

} // namespace algorihtm
} // namespace brush

